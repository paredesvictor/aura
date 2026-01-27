#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCBundle.h>
#include <Wire.h>
#include <Adafruit_BNO055.h>
#include "esp_sleep.h"

// IO 
int led = 15;
gpio_num_t buttonPin = GPIO_NUM_9;

// Sensor
uint16_t BNO055_SAMPLERATE_DELAY_MS = 10;
Adafruit_BNO055 bno = Adafruit_BNO055(55);


// WiFi configuration
const char* ssid = "SFR_CCD0";
const char* password = "baguette18";
IPAddress local_IP(192, 168, 1, 18);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(1, 1, 1, 1);

// OSC destination
const char* destIP = "192.168.1.71";       // Receiving device IP (adjust if needed)
const int destPort = 8011;              // Receiving port
const int localPort = 12345;            // Port to send from

WiFiUDP Udp;

// === Sensor & Processing Setup ===

void setup() {
  Serial.begin(115200);

  pinMode(buttonPin, INPUT);
  gpio_wakeup_enable(buttonPin, GPIO_INTR_LOW_LEVEL);
  esp_sleep_enable_gpio_wakeup();

  pinMode(led,OUTPUT);
  startupBlink(led);
  
  connectToWiFi();
  Udp.begin(localPort);

  if(!bno.begin())
  {
    /* There was a problem detecting the BNO055 ... check your connections */
    Serial.print("Ooops, no BNO055 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
  bno.setExtCrystalUse(true);
}

// Main loop //

void loop() {

  if (digitalRead(buttonPin) == LOW) {
    // sleep
    Serial.println("entering sleep mode...");
    sleepBlink(led);
    esp_light_sleep_start();   
    // after wake
    while (digitalRead(buttonPin) == LOW) delay(10);
    sleepBlink(led);
    WiFi.mode(WIFI_OFF);
    delay(100);
    WiFi.mode(WIFI_STA);
    connectToWiFi();
    Udp.begin(localPort);
  }

  // get battery voltage and sensors values
  int voltage = analogReadMilliVolts(0);
  imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  imu::Vector<3> accel = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);
  imu::Vector<3> gyro = bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);

  // prepare OSC bundle
  OSCBundle bundle;
  bundle.add("/orientation").add((float) euler.x()).add((float) euler.y()).add((float) euler.z());
  bundle.add("/accelerometer").add((float) accel.x()).add((float) accel.y()).add((float) accel.z());
  bundle.add("/gyroscope").add((float) gyro.x()).add((float) gyro.y()).add((float) gyro.z());
  bundle.add("/battery").add((float) 2 * voltage / 1000);

  // Send OSC Bundle over UDP
  Udp.beginPacket(destIP, destPort);
  bundle.send(Udp);
  Udp.endPacket();
  bundle.empty();
}

void startupBlink(int led) {
  digitalWrite(led,HIGH);
  delay(200);
  digitalWrite(led,LOW);
  delay(200);
  digitalWrite(led,HIGH);
  delay(200);
  digitalWrite(led,LOW);
  delay(200);
  digitalWrite(led,HIGH);
  delay(200);
  digitalWrite(led,LOW);
}

void sleepBlink(int led) {
  digitalWrite(led,HIGH);
  delay(200);
  digitalWrite(led,LOW);
  delay(200);
  digitalWrite(led,HIGH);
  delay(200);
  digitalWrite(led,LOW);
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
    if (attempts %2 == 0) {
      digitalWrite(led,HIGH);
    } else {
    digitalWrite(led,LOW);
    }
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(led,HIGH);
  } else {
    Serial.println("\nFailed to connect to WiFi.");
    digitalWrite(led,LOW);
    while (true); // Stop execution
  }
}