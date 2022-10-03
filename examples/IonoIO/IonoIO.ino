/*
  IonoIO.ino - Using Iono's I/O

    Copyright (C) 2022 Sfera Labs S.r.l. - All rights reserved.

    For information, see:
    https://www.sferalabs.cc/

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#include <Iono.h>

unsigned long printTs;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Iono.setup();

  // Set callback function on AV1 state change
  // with 500 ms min stable time and 200 mV min variation
  Iono.subscribeAnalog(AV1, 500, 0.2, onAV1change);

  // Set callback function on AI2 state change
  // with 300 ms min stable time and 1 mA min variation
  Iono.subscribeAnalog(AI2, 300, 1, onAI2change);

  // Set same callback function on DI3 and DI4 state change
  // with 100 and 200 ms debounce
  Iono.subscribeDigital(DI3, 100, onDebounce);
  Iono.subscribeDigital(DI4, 200, onDebounce);

  // Flip DO2 on every low-to-high transition of DI3
  // after a 100 ms debounce (must be same as subscribe)
  Iono.linkDiDo(DI3, DO2, LINK_FLIP_H, 100);

  // If DI5 and/or DI6 are used as TTL lines (jumper in BYP position)
  // call setBypass() and set their pin mode
  Iono.setBypass(DI5, INPUT);
  Iono.setBypass(DI6, OUTPUT);

  Serial.println("I/O setup done.");
}

void loop() {
  // Call Iono.process() with intervals smaller than
  // any debounce or stable times set with any subscribe
  // or link functions
  Iono.process();

  if (millis() - printTs > 2000) {
    Iono.flip(DO1);

    Iono.flip(DI6);

    if (Iono.read(DI3) == HIGH) {
      Iono.write(DO3, HIGH);
      Iono.write(AO1, 5);
    } else {
      Iono.write(DO3, LOW);
      Iono.write(AO1, 0.5);
    }

    Serial.println("-------------");

    Serial.print("AV1 = ");
    Serial.print(Iono.read(AV1));
    Serial.println(" V");

    Serial.print("AI2 = ");
    Serial.print(Iono.read(AI2));
    Serial.println(" mA");

    Serial.print("DI3 = ");
    Serial.println(Iono.read(DI3) == HIGH ? "high" : "low");

    Serial.print("DI4 = ");
    Serial.println(Iono.read(DI4) == HIGH ? "high" : "low");

    Serial.print("DO1 = ");
    Serial.println(Iono.read(DO1) == HIGH ? "high" : "low");

    Serial.print("DI5 = ");
    Serial.println(Iono.read(DI5) == HIGH ? "high" : "low");

    Serial.print("DI6 = ");
    Serial.println(Iono.read(DI6) == HIGH ? "high" : "low");

    printTs = millis();
  }

  delay(10);
}

void onAV1change(uint8_t pin, float val) {
  Serial.print("AV1 change = ");
  Serial.print(val);
  Serial.println(" V");
}

void onAI2change(uint8_t pin, float val) {
  Serial.print("AI2 change = ");
  Serial.print(val);
  Serial.println(" mA");
}

void onDebounce(uint8_t pin, float val) {
  if (pin == DI3) {
    Serial.print("DI3");
  } else if (pin == DI4) {
    Serial.print("DI4");
  }
  Serial.print(" debounce = ");
  Serial.println(val == HIGH ? "high" : "low");
}
