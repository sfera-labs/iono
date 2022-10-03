/*
  UDPSimple.ino - Arduino sketch showing the use of the IonoUDP library

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
#include <IonoUDP.h>

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0xD5, 0x8C };

IPAddress ip(192, 168, 1, 243);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

unsigned int port = 7878;

EthernetUDP Udp;

void setup() {
  Ethernet.begin(mac, ip, gateway, subnet);
  Udp.begin(port);
  IonoUDP.begin("myIono", Udp, port, 50, 0.1);
}

void loop() {
  IonoUDP.process();
  delay(10);
}
