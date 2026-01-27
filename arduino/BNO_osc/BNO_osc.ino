#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCBundle.h>
#include <OSCMessage.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

Adafruit_BNO055 bno = Adafruit_BNO055(55);

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
const int destPort = 8014;              // Receiving port
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
  connectToWiFi();
  Udp.begin(localPort);

  if(!bno.begin())
  {
    /* There was a problem detecting the BNO055 ... check your connections */
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    ESP.restart();
  }

  delay(1000);
    
  bno.setExtCrystalUse(true);
}

// Main loop //

void loop() {
  /* Get a new sensor event */ 
  sensors_event_t event; 
  bno.getEvent(&event);
  
  /* Display the floating point data */
  Serial.print("X: ");
  Serial.print(event.orientation.x, 4);
  Serial.print("\tY: ");
  Serial.print(event.orientation.y, 4);
  Serial.print("\tZ: ");
  Serial.print(event.orientation.z, 4);
  Serial.println("");

  OSCMessage msg("/euler");
  msg.add(event.orientation.x);
  msg.add(event.orientation.y);
  msg.add(event.orientation.z);
  Udp.beginPacket(destIP, destPort);
    msg.send(Udp); // send the bytes to the SLIP stream
  Udp.endPacket(); // mark the end of the OSC Packet
  msg.empty(); //
  // imu::Vector<3> accel = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);
  // imu::Vector<3> linear = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
  // imu::Vector<3> gravity = bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY);
  // imu::Vector<3> gyro = bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);
  // imu::Vector<3> magneto = bno.getVector(Adafruit_BNO055::VECTOR_MAGNETOMETER);
  // imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  // imu::Quaternion quat = bno.getQuat();

  // Serial.print(euler.x());
  // Serial.print(",");
  // Serial.print(euler.y());
  // Serial.print(",");
  // Serial.println(euler.z());

  // OSCMessage msg("/euler");
  // msg.add(euler.x());
  // msg.add(euler.y());
  // msg.add(euler.z());
  // Udp.beginPacket(destIP, destPort);
  //   msg.send(Udp); // send the bytes to the SLIP stream
  // Udp.endPacket(); // mark the end of the OSC Packet
  // msg.empty(); // empty the bundle to free room for a new one

  // OSCBundle bndl;
  // bndl.add("/acceleration").add(euler.x()).add(euler.y()).add(euler.z());
  // // bndl.add("/linear-acceleration").add(linear.x()).add(linear.y()).add(linear.z());
  // // bndl.add("/gravity").add(gravity.x()).add(gravity.y()).add(gravity.z());
  // // bndl.add("/gyroscope").add(gyro.x()).add(gyro.y()).add(gyro.z());
  // // bndl.add("/magnetometer").add(magneto.x()).add(magneto.y()).add(magneto.z());
  // // bndl.add("/euler").add(euler.x()).add(euler.y()).add(euler.z());
  // // bndl.add("/quaternion").add(quat.w()).add(quat.x()).add(quat.y()).add(quat.z());
  // Udp.beginPacket(destIP, destPort);
  //   bndl.send(Udp); // send the bytes to the SLIP stream
  // Udp.endPacket(); // mark the end of the OSC Packet
  // bndl.empty(); // empty the bundle to free room for a new one

  delay(10);
}

  
