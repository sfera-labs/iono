/*
  OneWireTempSens.ino - 1-Wire Dallas temperature sensors example

    Copyright (C) 2022-2025 Sfera Labs S.r.l. - All rights reserved.

    For information, see:
    https://www.sferalabs.cc/

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.

  N.B.: Use 5V-compatible 1-Wire sensors

  This sketch requires the "OneWire" and "DallasTemperature" Arduino libraries

  Usage:
  - Place the DI6 internal jumper into BYP position
  - Connect the 5V (red/brown) wire of the sensor(s) to 5VO on Iono MKR, or to AO1 on Iono Uno
  - Connect the ground (black) wire to GND and the data (yellow/blue/...) wire to DI6
  - On Iono Uno you may need a pull-up resistor between DI6 and AO1 (10K should work)
  - Run the sketch and check the output in the Serial Monitor
*/

#include <Iono.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define MAX_SENSORS 20

OneWire oneWire(IONO_PIN_DI6_BYP);
DallasTemperature sensors(&oneWire);

DeviceAddress addrs[MAX_SENSORS];

int n = 0;

void setup(void) {
  Iono.setup();

#ifdef IONO_UNO
  Iono.write(AO1, 5);
#endif

  Serial.begin(9600);
  while(!Serial);

  Serial.println("=== Iono 1-Wire Dallas temperature sensors demo ===");

  do {
    sensors.begin();

    n = sensors.getDeviceCount();
    if (n > MAX_SENSORS) {
      n = MAX_SENSORS;
    }

    Serial.print("Found ");
    Serial.print(n);
    Serial.println(" sensor(s).");

    delay(500);
  } while (n <= 0);

  for (int i = 0; i < n; i++) {
    sensors.getAddress(addrs[i], i);
  }
}

void loop(void) {
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures();
  Serial.println(" done!");

  for (int i = 0; i < n; i++) {
    printAddress(addrs[i]);
    Serial.print(": ");
    Serial.println(sensors.getTempC(addrs[i]));
  }

  delay(1000);
}

void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
