#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <Wire.h>

// Sensor Configuration 
byte sampleRate = 1000;

// WiFi configuration
const char* ssid = "AURA_SIDE2";
const char* password = "aura2025";

IPAddress local_IP(192, 168, 0, 47);     // Must be inside DHCP range
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(1, 1, 1, 1);

// OSC destination
const char* destIP = "192.168.0.100";       // Receiving device IP (adjust if needed)
const int destPort = 8011;              // Receiving port
const int localPort = 12345;            // Port to send from

WiFiUDP Udp;

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
  Udp.begin(localPort);
}

// Main loop //

void loop() {
    int32_t v1 = analogRead(1);
    int32_t v2 = analogRead(4);
    int32_t v3 = analogRead(6);
    int32_t v4 = analogRead(5);

    Serial.print(v1);
    Serial.print(",");
    Serial.print(v2);
    Serial.print(",");
    Serial.print(v3);
    Serial.print(",");
    Serial.println(v4);

  // Send OSC message with three values: raw IR, temperature, beatDetected
    OSCMessage msg("/gant");
    msg.add((int32_t)v1);
    msg.add((int32_t)v2);
    msg.add((int32_t)v3);
    msg.add((int32_t)v4);
    Udp.beginPacket(destIP, destPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();

    delay(10);
}

  
