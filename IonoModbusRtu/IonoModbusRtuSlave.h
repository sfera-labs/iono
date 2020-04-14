/*
  IonoModbusRtuSlave.h - Modbus RTU Slave library for Iono Arduino and Iono MKR

    Copyright (C) 2018 Sfera Labs S.r.l. - All rights reserved.

    For information, see the iono web site:
    http://www.sferalabs.cc/iono-arduino

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#ifndef IonoModbusRtuSlave_h
#define IonoModbusRtuSlave_h

#include <ModbusRtuSlave.h>
#include <Iono.h>

#define MB_RESP_PASS 0xFE

class IonoModbusRtuSlaveClass {
  public:
    static void begin(byte unitAddr, unsigned long baud, unsigned long config, unsigned long diDebounceTime);
    static void process();
    static void setCustomHandler(ModbusRtuSlaveClass::Callback *callback);
    static void setInputMode(int idx, char mode);
    static void subscribeDigital(uint8_t pin, IonoClass::Callback *callback);

  private:
    static bool _di1deb;
    static bool _di2deb;
    static bool _di3deb;
    static bool _di4deb;
    static bool _di5deb;
    static bool _di6deb;

    static word _di1count;
    static word _di2count;
    static word _di3count;
    static word _di4count;
    static word _di5count;
    static word _di6count;

    static IonoClass::Callback *_di1Callback;
    static IonoClass::Callback *_di2Callback;
    static IonoClass::Callback *_di3Callback;
    static IonoClass::Callback *_di4Callback;
    static IonoClass::Callback *_di5Callback;
    static IonoClass::Callback *_di6Callback;

    static char _inMode[4];

    static ModbusRtuSlaveClass::Callback *_customCallback;

    static byte onRequest(byte unitAddr, byte function, word regAddr, word qty, byte *data);
    static void onDIChange(uint8_t pin, float value);
    static bool checkAddrRange(word regAddr, word qty, word min, word max);
    static uint8_t indexToDO(int i);
    static uint8_t indexToDI(int i);
    static bool indexToDIdeb(int i);
    static word indexToDIcount(int i);
    static uint8_t indexToAV(int i);
    static uint8_t indexToAI(int i);
};

extern IonoModbusRtuSlaveClass IonoModbusRtuSlave;

#endif
