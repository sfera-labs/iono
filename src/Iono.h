/*
  Iono.h - Arduino library for Iono Uno/MKR/RP

    Copyright (C) 2014-2022 Sfera Labs S.r.l. - All rights reserved.

    For information, see:
    https://www.sferalabs.cc/

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

#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_SAMD_ZERO) || defined(ARDUINO_AVR_UNO_WIFI_REV2)
#define IONO_ARDUINO 1
#define IONO_UNO 1
#elif defined(ARDUINO_ARCH_RP2040)
#define IONO_RP 1
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

#ifdef IONO_UNO
  #define IONO_PIN_DO1 A4
  #define IONO_PIN_DO2 A5
  #define IONO_PIN_DO3 5
  #define IONO_PIN_DO4 6
  #define IONO_PIN_DO5 7
  #define IONO_PIN_DO6 8
  #define DO_IDX_MAX 6

  #define IONO_PIN_DI1 A0
  #define IONO_PIN_AV1 A0
  #define IONO_PIN_AI1 A0

  #define IONO_PIN_DI2 A1
  #define IONO_PIN_AV2 A1
  #define IONO_PIN_AI2 A1

  #define IONO_PIN_DI3 A2
  #define IONO_PIN_AV3 A2
  #define IONO_PIN_AI3 A2

  #define IONO_PIN_DI4 A3
  #define IONO_PIN_AV4 A3
  #define IONO_PIN_AI4 A3

  #define IONO_PIN_DI5 2
  #define IONO_PIN_DI6 3
  #define IONO_PIN_DI5_BYP 2
  #define IONO_PIN_DI6_BYP 3

  #define IONO_PIN_AO1 9
#elif defined(IONO_RP)
  #define IONO_PIN_DO1 13
  #define IONO_PIN_DO2 12
  #define IONO_PIN_DO3 11
  #define IONO_PIN_DO4 10
  #define DO_IDX_MAX 4

  #define IONO_PIN_DI1 26
  #define IONO_PIN_AV1 26
  #define IONO_PIN_AI1 26

  #define IONO_PIN_DI2 27
  #define IONO_PIN_AV2 27
  #define IONO_PIN_AI2 27

  #define IONO_PIN_DI3 28
  #define IONO_PIN_AV3 28
  #define IONO_PIN_AI3 28

  #define IONO_PIN_DI4 29
  #define IONO_PIN_AV4 29
  #define IONO_PIN_AI4 29

  #define IONO_PIN_DI5 24
  #define IONO_PIN_DI6 23
  #define IONO_PIN_DI5_BYP 7
  #define IONO_PIN_DI6_BYP 6

  #define IONO_PIN_AO1 8
#else
  #define IONO_PIN_DO1 3
  #define IONO_PIN_DO2 2
  #define IONO_PIN_DO3 A6
  #define IONO_PIN_DO4 A5
  #define DO_IDX_MAX 4

  #define IONO_PIN_DI1 A1
  #define IONO_PIN_AV1 A1
  #define IONO_PIN_AI1 A1

  #define IONO_PIN_DI2 A2
  #define IONO_PIN_AV2 A2
  #define IONO_PIN_AI2 A2

  #define IONO_PIN_DI3 A3
  #define IONO_PIN_AV3 A3
  #define IONO_PIN_AI3 A3

  #define IONO_PIN_DI4 A4
  #define IONO_PIN_AV4 A4
  #define IONO_PIN_AI4 A4

  #define IONO_PIN_DI5 7
  #define IONO_PIN_DI6 5
  #define IONO_PIN_DI5_BYP 0
  #define IONO_PIN_DI6_BYP 1

  #define IONO_PIN_AO1 A0
#endif

#ifdef IONO_MKR
#define PIN_TXEN 4
#endif

#ifdef IONO_RP
#define PIN_TXEN_N 25
#define SERIAL_PORT_MONITOR Serial
#define SERIAL_PORT_HARDWARE Serial1
#endif

#define IONO_RS485 SERIAL_PORT_HARDWARE

#define LINK_FOLLOW 1
#define LINK_INVERT 2
#define LINK_FLIP_T 3
#define LINK_FLIP_H 4
#define LINK_FLIP_L 5

#if (ARDUINO_API_VERSION >= 10000)
typedef PinMode iono_pin_mode_t;
#else
typedef int iono_pin_mode_t;
#endif

class IonoClass
{
  public:
    typedef void Callback(uint8_t pin, float value);
    IonoClass();
#if defined(IONO_MKR) || defined(IONO_RP)
    void setBYP(uint8_t pin, bool value);
#endif
    void setBypass(uint8_t pin, iono_pin_mode_t mode);
    void setup();
    float read(uint8_t pin);
    float readAnalogAvg(uint8_t pin, int n);
    void write(uint8_t pin, float value);
    void flip(uint8_t pin);
    void subscribeDigital(uint8_t pin, unsigned long stableTime, Callback *callback);
    void subscribeAnalog(uint8_t pin, unsigned long stableTime, float minVariation, Callback *callback);
    void linkDiDo(uint8_t dix, uint8_t dox, uint8_t mode, unsigned long stableTime);
    void process();
    void serialTxEn(bool enabled);

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
