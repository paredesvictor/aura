#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <Wire.h>
#include "EMGFilters.h"
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define SensorInputPin 5 // input pin number

EMGFilters myFilter;
int sampleRate = SAMPLE_FREQ_500HZ;
int humFreq = NOTCH_FREQ_50HZ;

int threshold = 0;

const int bufferSize = 256;
float buffer[bufferSize];
int sampleCounter = 0;
const int processEveryNSamples = 20; // Adjust to reduce compute frequency
float lastFilteredValue = 0.0;

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

void rootMeanSquare(float* signal, int len, int windowSize) {
  float* temp = new float[len];
  for (int i = 0; i < len; i++) {
    float sum = 0;
    int count = 0;
    for (int j = i - windowSize; j <= i + windowSize; j++) {
      if (j >= 0 && j < len) {
        sum += signal[j] * signal[j];
        count++;
      }
    }
    temp[i] = sqrt(sum / count);
  }
  for (int i = 0; i < len; i++) signal[i] = temp[i];
  delete[] temp;
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
  Udp.begin(localPort);

  for (int i = 0; i < bufferSize; i++) buffer[i] = 0; // init buffer

  myFilter.init(sampleRate, humFreq, true, true, true);
}

// Main loop //

void loop() {
  int value = analogRead(SensorInputPin);
  value = myFilter.update(value);

  // for (int i = 0; i < bufferSize - 1; i++) {
  //   buffer[i] = buffer[i + 1];
  // }
  // buffer[bufferSize - 1] = float(value);

  // sampleCounter++;
  // if (sampleCounter >= processEveryNSamples) {
    // sampleCounter = 0;
    // float tempBuffer[bufferSize];
    // memcpy(tempBuffer, buffer, sizeof(buffer));
    // rootMeanSquare(tempBuffer, bufferSize, 30);
    // lastFilteredValue = tempBuffer[bufferSize - 1];
    // Serial.println(lastFilteredValue);

    // OSCMessage msg("/emg");
    // msg.add(int32_t(value));
    // Udp.beginPacket(destIP, destPort);
    // msg.send(Udp);
    // Udp.endPacket();
    // msg.empty();
  // }
  OSCMessage msg("/emg");
  msg.add(int32_t(value));
  Udp.beginPacket(destIP, destPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();

  // if (millis() < 5000) {
    // sampleCounter++;
    // if (sampleCounter > 100) {
    //   threshold = (value > threshold) ? value : threshold;
    // }
    // Serial.println(value);
  // } else {
    // Serial.print("Threshold:");
    // Serial.print(threshold);
    // Serial.print(",Pre-Value:");
    // Serial.print(value);
    // value = (value > threshold) ? value : 0;
    // Serial.print(",Value:");
    // Serial.println(value);
  // }
  
  
  delayMicroseconds(200);
}

  

