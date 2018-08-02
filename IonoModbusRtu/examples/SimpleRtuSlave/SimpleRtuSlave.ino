/*  
  SimpleRtuSlave.ino - Arduino sketch showing the use of the IonoModbusRtuSlave library

    Copyright (C) 2018 Sfera Labs S.r.l. - All rights reserved.

    For information, see the iono web site:
    http://www.sferalabs.cc/iono-arduino
  
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#include <IonoModbusRtuSlave.h>

void setup() {
  /**
   * Start the modbus server with: 
   * unit address: 10
   * baud rate: 115200
   * data bits: 8
   * parity: even
   * stop bits: 1
   * debounce time on digital inputs: 50ms
   */
  IonoModbusRtuSlave.begin(10, 115200, SERIAL_8E1, 50);
}

void loop() {
  /**
   * Process requests
   */
  IonoModbusRtuSlave.process();
}


