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
/     turns on relay DO1 and DO2 and sets a
/     5.30V voltage on the analog autput AO1
/
/ http://192.168.1.243/api/subscribe?pin=DI1&st=500&host=192.168.1.242&port=8080&cmd=/foo?$pin=$val
/     Every time DI1 changes value
/     and is stable for 500ms (st=500)
/     execute an HTTP GET request to
/     "192.168.1.242:8080/foo?$pin=$val"
/     where "$pin" is substituted by the 
/     name of the pin (i.e. "DI1") and
/     "$val" by the current value of 
/     the pin (i.e. 1 or 0)
/
/ http://192.168.1.243/api/subscribe?pin=AV2&mv=1&host=192.168.1.242&port=8080&cmd=/bar?$pin=$val
/     Every time the voltage on AV2
/     changes of a value >= 1V (mv=1)
/     execute an HTTP GET request to
/     "192.168.1.242:8080/bar?$pin=$val"
/     where "$pin" is substituted by the 
/     name of the pin (i.e. "AV2") and
/     "$val" by the current value of 
/     the pin (e.g. "5.40")
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
