/*
  RS485Echo.ino - Arduino sketch showing the use of the RS-485 interface.

    Copyright (C) 2014-2020 Sfera Labs S.r.l. - All rights reserved.

    For information, see the iono web site:
    http://www.sferalabs.cc/iono

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.

  With this sketch Iono Arduino or Iono MKR reads whatever is sent on the
  RS-485 port and echoes it back.
*/

#include <Iono.h>

#define MAX_LEN 512

byte rxBuff[MAX_LEN + 1];
int rxIdx;

void setup() {
  Iono.setup();

  /**
   * Init port
   * baud rate: 19200
   * data bits: 8
   * parity: none
   * stop bits: 2
   */
  IONO_RS485.begin(19200, SERIAL_8N2);
}

void loop() {
  if (IONO_RS485.available() > 0) {
    rxIdx = 0;

    // Read into buffer while data is available
    while(IONO_RS485.available() > 0 && rxIdx <= MAX_LEN) {
      rxBuff[rxIdx++] = IONO_RS485.read();
      if (IONO_RS485.available() == 0) {
        // give it some extra time to check if
        // any other data is on its way...
        delay(50);
      }
    }
    
    Iono.serialTxEn(true);

    IONO_RS485.write(rxBuff, rxIdx);
    IONO_RS485.flush();

    Iono.serialTxEn(false);
  }
  
  delay(50);
}
