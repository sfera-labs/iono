/*
  WebSubscribe.ino - IonoWeb library example for Iono Uno Ethernet

    Copyright (C) 2014-2022 Sfera Labs S.r.l. - All rights reserved.

    For information, see:
    https://www.sferalabs.cc/

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#include <SPI.h>
#include <Ethernet.h>
#include <Iono.h>
#include <IonoWeb.h>

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0xD5, 0x8C };

IPAddress ip(192, 168, 1, 243);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

void setup() {
  Iono.setup();
  Ethernet.begin(mac, ip, gateway, subnet);
  IonoWeb.begin(80);

  /*
  / Every time a pin changes value
  / and is stable for 100ms
  / execute ah HTTP GET request to
  / "192.168.1.242:8080/bar?<pin>=<val>"
  / where <pin> is substituted by the
  / name of the pin (i.e. "DI1" or "AV3") and
  / <val> by the current value of
  / the pin.
  / For inputs read as voltage or current
  / the call will be triggered only if the
  / value changes of more than 0.1 V or mA
  / Input 1 will be read as digital (1),
  / Input 2 will be read as digital (1),
  / Input 3 will be read as voltage (2),
  / Input 4 will be read as current (3).
  / Request examples:
  / http://192.168.1.242:8080/bar?DI1=1
  / http://192.168.1.242:8080/bar?AV3=5.30
  */
  IonoWeb.subscribe(100, 0.1, "192.168.1.242", 8080, "/bar", 1, 1, 2, 3);
}

void loop() {
  // Check all the inputs
  Iono.process();
}
