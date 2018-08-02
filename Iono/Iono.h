/*
  Iono.h - Arduino library for the control of iono

    Copyright (C) 2014-2018 Sfera Labs S.r.l. - All rights reserved.

    For information, see the iono web site:
    http://www.sferalabs.cc/iono-arduino

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#ifndef Iono_h
#define Iono_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_SAMD_ZERO)
#define IONO_ARDUINO 1
#else
#define IONO_MKR 1
#endif

#define DO1 0
#define DO2 1
#define DO3 2
#define DO4 3
#define DO5 4
#define DO6 5

#define DI1 6
#define AV1 7
#define AI1 8

#define DI2 9
#define AV2 10
#define AI2 11

#define DI3 12
#define AV3 13
#define AI3 14

#define DI4 15
#define AV4 16
#define AI4 17

#define DI5 18
#define DI6 19
#define AO1 20

#ifdef IONO_MKR
#define PIN_TXEN 4
#endif

#define LINK_FOLLOW 1
#define LINK_INVERT 2
#define LINK_FLIP_T 3
#define LINK_FLIP_H 4
#define LINK_FLIP_L 5

class IonoClass
{
  public:
    typedef void Callback(uint8_t pin, float value);
    IonoClass();
#ifdef IONO_MKR
    void setBYP(uint8_t pin, bool value);
#endif
    float read(uint8_t pin);
    void write(uint8_t pin, float value);
    void flip(uint8_t pin);
    void subscribeDigital(uint8_t pin, unsigned long stableTime, Callback *callback);
    void subscribeAnalog(uint8_t pin, unsigned long stableTime, float minVariation, Callback *callback);
    void linkDiDo(uint8_t dix, uint8_t dox, uint8_t mode, unsigned long stableTime);
    void process();

  private:
    uint8_t _pinMap[21];
    float _ao1_val;
    typedef struct CallbackMap
    {
      uint8_t pin;
      unsigned long stableTime;
      float minVariation;
      Callback *callback;
      uint8_t linkedPin;
      uint8_t linkMode;
      float value;
      unsigned long lastTS;
    } CallbackMap;
    CallbackMap _i1;
    CallbackMap _i2;
    CallbackMap _i3;
    CallbackMap _i4;
    CallbackMap _i5;
    CallbackMap _i6;
    CallbackMap _o1;
    CallbackMap _o2;
    CallbackMap _o3;
    CallbackMap _o4;
    CallbackMap _o5;
    CallbackMap _o6;
    CallbackMap _a1;

    void check(CallbackMap *input);
};

extern IonoClass Iono;

#endif
