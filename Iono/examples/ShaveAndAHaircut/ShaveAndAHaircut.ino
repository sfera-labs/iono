/*  
  ShaveAndAHaircut.ino - Arduino sketch showing the use of the Iono library

    Copyright (C) 2014-2015 Sfera Labs, a division of Home Systems Consulting S.p.A. - All rights reserved.

    For information, see the iono web site:
    http://www.iono.cc/
  
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#include <Iono.h>

void setup() {
}

void loop() {
  shaveAndAHaircut();
  delay(2000);
}

void shaveAndAHaircut() {
  Iono.flip(DO1);
  delay(350);
  Iono.flip(DO2);
  delay(100);
  Iono.flip(DO3);
  delay(150);
  Iono.flip(DO4);
  delay(300);
  Iono.flip(DO5);
  delay(600);
  Iono.flip(DO6);
  delay(300);
  Iono.flip(DO1);
}
