#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCBundle.h>
#include <Wire.h>
#include <Adafruit_LSM6DS3TRC.h>

Adafruit_LSM6DS3TRC lsm6ds3trc;

// WiFi configuration
const char* ssid = "SFR_CCD0";
const char* password = "baguette18";

IPAddress local_IP(192, 168, 1, 72);  // Must be inside DHCP range
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(1, 1, 1, 1);

// OSC destination
const char* destIP = "192.168.1.70";  // Receiving device IP (adjust if needed)
const int destPort = 8013;             // Receiving port
const int localPort = 12345;           // Port to send from

WiFiUDP Udp;

// === Sensor & Processing Setup ===

void setup() {
  Serial.begin(115200);
  delay(1000);  // Let peripherals initialize properly
  connectToWiFi();
  Udp.begin(localPort);

  pinMode(NEOPIXEL_I2C_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_I2C_POWER, LOW);

  if (!lsm6ds3trc.begin_I2C()) {
    Serial.println("Failed to find LSM6DS3TR-C chip");
    while (1) {
      delay(10);
    }
  }

  lsm6ds3trc.configInt1(false, false, true); // accelerometer DRDY on INT1
  lsm6ds3trc.configInt2(false, true, false); // gyro DRDY on INT2
}

// Main loop //

void loop() {
  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;
  lsm6ds3trc.getEvent(&accel, &gyro, &temp);

  int32_t v1 = analogRead(1);
  int32_t v2 = analogRead(4);
  int32_t v3 = analogRead(6);
  int32_t v4 = analogRead(5);

  // Send OSC message with three values: raw IR, temperature, beatDetected
  OSCBundle bndl;
  bndl.add("/gant/accelerometer").add((float) accel.acceleration.x).add((float) accel.acceleration.y).add((float) accel.acceleration.z);
  bndl.add("/gant/gyroscope").add((float) gyro.gyro.x).add((float) gyro.gyro.y).add((float) gyro.gyro.z);
  bndl.add("/gant/doigts").add((int32_t)v1).add((int32_t)v2).add((int32_t)v3).add((int32_t)v4);
  Udp.beginPacket(destIP, destPort);
    bndl.send(Udp);
  Udp.endPacket();
  bndl.empty();

  delay(10);
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
    while (true)
      ;  // Stop execution
  }
}