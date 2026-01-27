#include "DFRobot_BloodOxygen_S.h"
#define I2C_COMMUNICATION
#define I2C_ADDRESS 0x57

DFRobot_BloodOxygen_S_I2C MAX30102(&Wire, I2C_ADDRESS);

void setup() {
  Serial.begin(115200);

  while (!MAX30102.begin()) {
    Serial.println("init fail!");
    ESP.restart();
  }
  Serial.println("init success!");
  Serial.println("start measuring...");
  MAX30102.sensorStartCollect();
}

void loop() {
  MAX30102.getHeartbeatSPO2();
  Serial.print("SPO2: ");
  Serial.print(MAX30102._sHeartbeatSPO2.SPO2);
  Serial.println("%");
  Serial.print("Heart rate: ");
  Serial.print(MAX30102._sHeartbeatSPO2.Heartbeat);
  Serial.println(" bpm");
  Serial.print("Board temp: ");
  Serial.print(MAX30102.getTemperature_C());
  Serial.println(" C");
  delay(4000);  // data updates every 4s
}