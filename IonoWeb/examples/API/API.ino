/*
  API.ino - Arduino sketch showing the use of the IonoWeb library 

    Copyright (C) 2014-2015 Sfera Labs, a division of Home Systems Consulting S.p.A. - All rights reserved.

    For information, see the iono web site:
    http://www.iono.cc/
  
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

/*
/ This example allows for the control of
/ Iono using simple HTTP requests. 
/ Here some examples:
/
/ http://192.168.1.243/api/state
/     returns a json object representing the
/     current state of Iono's inputs and outputs
/
/ http://192.168.1.243/api/set?DO1=1&DO2=1&AO1=5.30
/     switches on relay DO1 and DO2 and sets a
/     5.30V voltage on analog autput AO1
/
/ http://192.168.1.243/api/subscribe?mv=0.1&st=100&host=192.168.1.242&port=8080&cmd=/bar&mode1=d&mode2=d&mode3=v&mode4=i
/     Every time a pin changes value
/     and is stable for 100ms (st=100)
/     execute ah HTTP GET request to
/     "192.168.1.242:8080/bar?<pin>=<val>"
/     where <pin> is substituted by the 
/     name of the pin (i.e. "DI1" or "AV3") and
/     <val> by the current value of 
/     the pin.
/     For inputs read as voltage or current
/     the call will be triggered only if the 
/     value changes of more than 0.1 V or mA (mv=0.1)
/     Input 1 will be read as digital (mode1=d),
/     Input 2 will be read as digital (mode2=d),
/     Input 3 will be read as voltage (mode3=v),
/     Input 4 will be read as current (mode4=i).
*/

#include <SPI.h>
#include <Ethernet.h>
#include <Iono.h>
#include <WebServer.h>
#include <IonoWeb.h>

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0xD5, 0x8C };

IPAddress ip(192, 168, 1, 243);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

void setup() {
  Ethernet.begin(mac, ip, gateway, subnet);
  IonoWeb.begin(80);
}

void loop() {
  // Process incoming requests
  IonoWeb.processRequest();
  // Check all the inputs - needed only for subscribe
  Iono.process();
}
