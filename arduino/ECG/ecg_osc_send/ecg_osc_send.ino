#include <DFRobot_BloodOxygen_S.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <Wire.h>

DFRobot_BloodOxygen_S_I2C MAX30102(&Wire ,0x57);

// Sensor Configuration 
byte sampleRate = 1000;

// WiFi configuration
const char* ssid = "AURA_MAIN";
const char* password = "aura2025";

IPAddress local_IP(192, 168, 0, 47);     // Must be inside DHCP range
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(1, 1, 1, 1);

// OSC destination
const char* destIP = "192.168.0.100";       // Receiving device IP (adjust if needed)
const int destPort = 8003;              // Receiving port
const int localPort = 12345;            // Port to send from

WiFiUDP Udp;

float highpassFilter(float input, float dt, float cutoff) {
  static float prevInput = 0, prevOutput = 0;
  float RC = 1.0 / (2.0 * PI * cutoff);
  float alpha = RC / (RC + dt);
  float output = alpha * (prevOutput + input - prevInput);
  prevInput = input;
  prevOutput = output;
  return output;
}

float lowpassFilter(float input, float dt, float cutoff) {
  static float prevOutput = 0;
  float RC = 1.0 / (2.0 * PI * cutoff);
  float alpha = dt / (RC + dt);
  float output = prevOutput + alpha * (input - prevOutput);
  prevOutput = output;
  return output;
}

float bandpassFilter(float input, float dt) {
  float hp = highpassFilter(input, dt, 0.5);    // remove < 0.5 Hz
  float bp = lowpassFilter(hp, dt, 150.0);      // remove > 150 Hz
  return bp;
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi.");
    while (true); // Stop execution
  }
}

// === Sensor & Processing Setup ===

void setup() {
  Serial.begin(115200);
  delay(1000); // Let peripherals initialize properly
  connectToWiFi();

  if (false == MAX30102.begin()) {
    Serial.println("MAX30102 not found. Check wiring/power.");
    Wire.end();           // Gracefully shut down the I2C bus
    delay(100);           // Give it time to settle
    ESP.restart();        // Soft reset the ESP32
  }


  Udp.begin(localPort);
}

// Main loop //

void loop() {

  const float dt = 1.0 / sampleRate;  // 1000 Hz sampling rate

  // Add new IR sample
  MAX30102.getHeartbeatSPO2();
  uint32_t irValue = MAX30102._sHeartbeatSPO2.Heartbeat;

  // Read temperature
  // float temperature = MAX30102.getTemperature_C();

  uint32_t value = bandpassFilter((float)irValue, dt);

  // Send OSC message with three values: raw IR, temperature, beatDetected
  OSCMessage msg("/signal");
  msg.add((int32_t)value);
  Udp.beginPacket(destIP, destPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();

  delay(1000);
}

  
