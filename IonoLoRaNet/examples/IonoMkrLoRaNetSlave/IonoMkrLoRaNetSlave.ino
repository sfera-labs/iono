#include <Arduino.h>
#include <LoRa.h>
#include <IonoLoRaNet.h>
#include <Iono.h>

byte siteId[] = "LOL";
byte cryptoKey[] = "16bytesSecretKey";

IonoLoRaLocalSlave slave = IonoLoRaLocalSlave(3);

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("Starting...");

  if (!LoRa.begin(868E6)) {
    Serial.println("LoRa init failed.");
    while (true);
  }

  LoRa.enableCrc();
  LoRa.setSyncWord(0x34);

  LoRaNet.init(siteId, sizeof(siteId) - 1, cryptoKey);

  Iono.subscribeDigital(DO1, 0, &IonoLoRaLocalSlave::subscribeCallback);
  Iono.subscribeDigital(DO2, 0, &IonoLoRaLocalSlave::subscribeCallback);
  Iono.subscribeDigital(DO3, 0, &IonoLoRaLocalSlave::subscribeCallback);
  Iono.subscribeDigital(DO4, 0, &IonoLoRaLocalSlave::subscribeCallback);

  Iono.subscribeAnalog(AO1, 0, 0, &IonoLoRaLocalSlave::subscribeCallback);

  Iono.subscribeDigital(DI1, 50, &IonoLoRaLocalSlave::subscribeCallback);
  Iono.subscribeDigital(DI2, 50, &IonoLoRaLocalSlave::subscribeCallback);
  Iono.subscribeAnalog(AV3, 50, 0.1, &IonoLoRaLocalSlave::subscribeCallback);
  Iono.subscribeAnalog(AI4, 50, 0.1, &IonoLoRaLocalSlave::subscribeCallback);
  Iono.subscribeDigital(DI5, 50, &IonoLoRaLocalSlave::subscribeCallback);
  Iono.subscribeDigital(DI6, 50, &IonoLoRaLocalSlave::subscribeCallback);


  Serial.print("LoRaNet slave ");
  Serial.println(slave.getAddr());
}

unsigned long ts = 0;

void loop() {
  slave.process();

  if (millis() - ts >= 1000) {
    Serial.println("---------");
    ts = millis();
  }
}
