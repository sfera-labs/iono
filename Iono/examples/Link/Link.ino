/*  
  Link.ino - Arduino sketch showing the use of the Iono library

    Copyright (C) 2018 Sfera Labs S.r.l. - All rights reserved.

    For information, see the iono web site:
    http://www.sferalabs.cc/iono-arduino
  
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#include <Iono.h>

void setup() {
  Iono.linkDiDo(DI1, DO1, LINK_FOLLOW, 100);
  Iono.linkDiDo(DI2, DO2, LINK_INVERT, 100);
  Iono.linkDiDo(DI3, DO3, LINK_FLIP_T, 100);
  Iono.linkDiDo(DI4, DO4, LINK_FLIP_H, 100);
}

void loop() {
  Iono.process();
  delay(20);
}

