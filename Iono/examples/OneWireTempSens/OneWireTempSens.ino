/*  
  OneWireTempSens.ino - 1-Wire Dallas temperature sensors example

    Copyright (C) 2021 Sfera Labs S.r.l. - All rights reserved.

    For information, see the iono web site:
    http://www.sferalabs.cc/iono
  
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.

  Usage:
  - Place the DI6 internal jumper into BYP position
  - Connect the 3 wires of the 1-Wire sensor(s) to 5VO (red/brown wire), GND (black wire) and DI6 (data wire)
  - Run the sketch and check the output in the Serial Monitor
*/

#include <Iono.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define MAX_SENSORS 20

OneWire oneWire(IONO_PIN_DI6_BYP);
DallasTemperature sensors(&oneWire);

DeviceAddress addrs[MAX_SENSORS];

int n;

void setup(void) {
  Iono.setup();
  
  Serial.begin(9600);
  while(!Serial);
  
  Serial.println("=== Iono 1-Wire Dallas temperature sensors demo ===");
  
  sensors.begin();
  
  n = sensors.getDeviceCount();
  if (n > MAX_SENSORS) {
    n = MAX_SENSORS;
  }

  Serial.print("Found ");
  Serial.print(n);
  Serial.println(" sensor(s).");
  
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
