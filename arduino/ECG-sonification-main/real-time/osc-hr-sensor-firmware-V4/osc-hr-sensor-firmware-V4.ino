#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <Wire.h>


const int bufferSize = 256;
float buffer[bufferSize];
int sampleCounter = 0;
const int processEveryNSamples = 1; // Adjust to reduce compute frequency
float lastFilteredIR = 0.0;

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

// === Preprocessing functions ===

void meanRemoval(float* signal, int len) {
  float sum = 0;
  for (int i = 0; i < len; i++) sum += signal[i];
  float mean = sum / len;
  for (int i = 0; i < len; i++) signal[i] -= mean;
}

void amplitudeNormalization(float* signal, int len) {
  float minVal = signal[0], maxVal = signal[0];
  for (int i = 1; i < len; i++) {
    if (signal[i] < minVal) minVal = signal[i];
    if (signal[i] > maxVal) maxVal = signal[i];
  }
  float range = maxVal - minVal;
  if (range == 0) return;
  for (int i = 0; i < len; i++) signal[i] = (signal[i] - minVal) / range;
}

void movingAverageFilter(float* signal, int len, int windowSize) {
  float* temp = new float[len];
  for (int i = 0; i < len; i++) {
    float sum = 0;
    int count = 0;
    for (int j = i - windowSize; j <= i + windowSize; j++) {
      if (j >= 0 && j < len) {
        sum += signal[j];
        count++;
      }
    }
    temp[i] = sum / count;
  }
  for (int i = 0; i < len; i++) signal[i] = temp[i];
  delete[] temp;
}

int detectPeak(const float* signal, int len, float threshold) {
  for (int i = 1; i < len - 1; i++) {
    if (signal[i] > signal[i - 1] && signal[i] > signal[i + 1] && signal[i] > threshold) {
      return i;
    }
  }
  return -1;
}

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

  for (int i = 0; i < bufferSize; i++) buffer[i] = 0; // init buffer

  Udp.begin(localPort);
}

// Main loop //

void loop() {

  const float dt = 1.0 / sampleRate;  // 1000 Hz sampling rate


  // Shift all values left (drop oldest)
  for (int i = 0; i < bufferSize - 1; i++) {
    buffer[i] = buffer[i + 1];
  }

  // Add new IR sample
  MAX30102.getHeartbeatSPO2();
  uint32_t irValue = MAX30102._sHeartbeatSPO2.Heartbeat;
  buffer[bufferSize - 1] = irValue;

  // Beat detection (returns true if a beat is detected)
  bool beatDetected = checkForBeat(irValue);

  // Read temperature
  float temperature = MAX30102.getTemperature_C();

  sampleCounter++;

  // Preprocess every N samples if you want (currently every sample)
  if (sampleCounter >= processEveryNSamples) {
    sampleCounter = 0;

    float tempBuffer[bufferSize];
    memcpy(tempBuffer, buffer, sizeof(buffer));

    // meanRemoval(tempBuffer, bufferSize);
    // amplitudeNormalization(tempBuffer, bufferSize);
    // movingAverageFilter(tempBuffer, bufferSize, 4);

    lastFilteredIR = tempBuffer[bufferSize - 1];

    lastFilteredIR = bandpassFilter((float)irValue, dt);

    // You can still print or debug if you want:
    Serial.print("IR: "); Serial.print(irValue);
    Serial.print(" | Filtered IR: "); Serial.println(lastFilteredIR);
    // Serial.print(" | Temp: "); Serial.print(temperature);
    // Serial.print(" | Beat: "); Serial.println(beatDetected);
  }

  // Send OSC message with three values: raw IR, temperature, beatDetected
  OSCMessage msg("/ecg");

  msg.add((int32_t)irValue);
  msg.add(lastFilteredIR);
  msg.add(temperature);         // float value
  msg.add((int32_t)beatDetected); // OSC has no boolean, so use int 0 or 1

  Udp.beginPacket(destIP, destPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();

  // delay(1); // ~1000Hz sampling
}
