/* 
  Iono.cpp - Arduino library for the control of iono

    Copyright (C) 2014-2015 Sfera Labs, a division of Home Systems Consulting S.p.A. - All rights reserved.

    For information, see the iono web site:
    http://www.iono.cc/
  
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#include "Iono.h"

IonoClass::IonoClass() {
  _pinMap[DO1] = A4;
  _pinMap[DO2] = A5;
  _pinMap[DO3] = 5;
  _pinMap[DO4] = 6;
  _pinMap[DO5] = 7;
  _pinMap[DO6] = 8;

  _pinMap[DI1] = A0;
  _pinMap[AV1] = A0;
  _pinMap[AI1] = A0;

  _pinMap[DI2] = A1;
  _pinMap[AV2] = A1;
  _pinMap[AI2] = A1;

  _pinMap[DI3] = A2;
  _pinMap[AV3] = A2;
  _pinMap[AI3] = A2;

  _pinMap[DI4] = A3;
  _pinMap[AV4] = A3;
  _pinMap[AI4] = A3;

  _pinMap[DI5] = 2;
  _pinMap[DI6] = 3;
  _pinMap[AO1] = 9;

  pinMode(_pinMap[DO1], OUTPUT);
  pinMode(_pinMap[DO2], OUTPUT);
  pinMode(_pinMap[DO3], OUTPUT);
  pinMode(_pinMap[DO4], OUTPUT);
  pinMode(_pinMap[DO5], OUTPUT);
  pinMode(_pinMap[DO6], OUTPUT);
  
  pinMode(_pinMap[DI1], INPUT);
  pinMode(_pinMap[DI2], INPUT);
  pinMode(_pinMap[DI3], INPUT);
  pinMode(_pinMap[DI4], INPUT);
  
  pinMode(_pinMap[DI5], INPUT);
  pinMode(_pinMap[DI6], INPUT);
  pinMode(_pinMap[AO1], OUTPUT);
}

void IonoClass::subscribeDigital(uint8_t pin, unsigned long stableTime, Callback *callback) {
  CallbackMap* input;

  switch (pin) {
    case DI1:
      input = &_i1;
      break;

    case DI2:
      input = &_i2;
      break;

    case DI3:
      input = &_i3;
      break;

    case DI4:
      input = &_i4;
      break;

    case DI5:
      input = &_i5;
      break;

    case DI6:
      input = &_i6;
      break;

    case DO1:
      input = &_o1;
      break;

    case DO2:
      input = &_o2;
      break;

    case DO3:
      input = &_o3;
      break;

    case DO4:
      input = &_o4;
      break;

    case DO5:
      input = &_o5;
      break;

    case DO6:
      input = &_o6;
      break;

    default:
      return;
  }

  (*input).pin = pin;
  (*input).stableTime = stableTime;
  (*input).minVariation = 0;
  (*input).callback = callback;
  (*input).lastValue = read(pin);
  (*input).value = read(pin);
  (*input).lastTS = 0;
}

void IonoClass::subscribeAnalog(uint8_t pin, unsigned long stableTime, float minVariation, Callback *callback) {
  CallbackMap* input;

  switch (pin) {
    case AV1:
    case AI1:
      input = &_i1;
      break;

    case AV2:
    case AI2:
      input = &_i2;
      break;

    case AV3:
    case AI3:
      input = &_i3;
      break;

    case AV4:
    case AI4:
      input = &_i4;
      break;

    default:
      return;
  }

  (*input).pin = pin;
  (*input).stableTime = stableTime;
  (*input).minVariation = minVariation;
  (*input).callback = callback;
  (*input).lastValue = read(pin);
  (*input).value = read(pin);
  (*input).lastTS = 0;
}

void IonoClass::process() {
  check(&_i1);
  check(&_i2);
  check(&_i3);
  check(&_i4);
  check(&_i5);
  check(&_i6);
  check(&_o1);
  check(&_o2);
  check(&_o3);
  check(&_o4);
  check(&_o5);
  check(&_o6);
}

void IonoClass::check(CallbackMap *input) {
  if ((*input).callback != NULL) {
    float val = read((*input).pin);
    unsigned long ts = millis();

    if (val != (*input).lastValue) {
      (*input).lastTS = ts;
    }
  
    if ((ts - (*input).lastTS) >= (*input).stableTime) {
      if (val != (*input).value) {
        float diff = (*input).value - val;
        diff = abs(diff);
        if (diff >= (*input).minVariation) {
          (*input).value = val;
          (*input).callback((*input).pin, val);
        }
      }
    }

    (*input).lastValue = val;
  }
}

float IonoClass::read(uint8_t pin) {
  if (pin >= DO1 && pin <= DO6) {
    return digitalRead(_pinMap[pin]);
  }
  
  if (pin == DI1 || pin == DI2 || pin == DI3 || pin == DI4 || pin == DI5 || pin == DI6) {
    return digitalRead(_pinMap[pin]);
  }
  
  if (pin == AV1 || pin == AV2 || pin == AV3 || pin == AV4) {
    return analogRead(_pinMap[pin]) * 10.0 / 1023.0;
  }
  
  if (pin == AI1 || pin == AI2 || pin == AI3 || pin == AI4) {
    return analogRead(_pinMap[pin]) * 20.0 / 1023.0;
  }
}

void IonoClass::write(uint8_t pin, float value) {
  if (pin >= DO1 && pin <= DO6) {
    digitalWrite(_pinMap[pin], value);
  }
  
  else if (pin == AO1) {
    analogWrite(_pinMap[pin], value * 255 / 10);
  }
}

void IonoClass::flip(uint8_t pin) {
  write(pin, read(pin) == HIGH ? LOW : HIGH);
}

IonoClass Iono;
