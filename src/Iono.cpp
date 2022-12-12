/*
  Iono.cpp - Arduino library for Iono Uno/MKR/RP

    Copyright (C) 2014-2022 Sfera Labs S.r.l. - All rights reserved.

    For information, see:
    https://www.sferalabs.cc/

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#include "Iono.h"

#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
#define ANALOG_READ_BITS 10
#define ANALOG_WRITE_BITS 8
#elif defined(IONO_RP)
#define ANALOG_READ_BITS 12
#define ANALOG_WRITE_BITS 16
#else
#define ANALOG_READ_BITS 12
#define ANALOG_WRITE_BITS 10
#endif

#ifdef IONO_ARDUINO
#define IONO_AV_MAX 10.0
#define IONO_AI_MAX 20.0
#else
#define IONO_AV_MAX 30.0
#define IONO_AI_MAX 25.0
#endif
#define IONO_AO_MAX 10.0

#define ANALOG_READ_MAX ((1 << ANALOG_READ_BITS) - 1)
#define ANALOG_WRITE_MAX ((1 << ANALOG_WRITE_BITS) - 1)

IonoClass::IonoClass() {
  _pinMap[DO1] = IONO_PIN_DO1;
  _pinMap[DO2] = IONO_PIN_DO2;
  _pinMap[DO3] = IONO_PIN_DO3;
  _pinMap[DO4] = IONO_PIN_DO4;
#ifdef IONO_PIN_DO5
  _pinMap[DO5] = IONO_PIN_DO5;
#endif
#ifdef IONO_PIN_DO6
  _pinMap[DO6] = IONO_PIN_DO6;
#endif

  _pinMap[DI1] = IONO_PIN_DI1;
  _pinMap[AV1] = IONO_PIN_AV1;
  _pinMap[AI1] = IONO_PIN_AI1;

  _pinMap[DI2] = IONO_PIN_DI2;
  _pinMap[AV2] = IONO_PIN_AV2;
  _pinMap[AI2] = IONO_PIN_AI2;

  _pinMap[DI3] = IONO_PIN_DI3;
  _pinMap[AV3] = IONO_PIN_AV3;
  _pinMap[AI3] = IONO_PIN_AI3;

  _pinMap[DI4] = IONO_PIN_DI4;
  _pinMap[AV4] = IONO_PIN_AV4;
  _pinMap[AI4] = IONO_PIN_AI4;

  _pinMap[DI5] = IONO_PIN_DI5;
  _pinMap[DI6] = IONO_PIN_DI6;
  _pinMap[AO1] = IONO_PIN_AO1;

  setup();
}

void IonoClass::setup() {
  pinMode(_pinMap[DO1], OUTPUT);
  pinMode(_pinMap[DO2], OUTPUT);
  pinMode(_pinMap[DO3], OUTPUT);
  pinMode(_pinMap[DO4], OUTPUT);
#ifdef IONO_ARDUINO
  pinMode(_pinMap[DO5], OUTPUT);
  pinMode(_pinMap[DO6], OUTPUT);
#endif

  pinMode(_pinMap[DI1], INPUT);
  pinMode(_pinMap[DI2], INPUT);
  pinMode(_pinMap[DI3], INPUT);
  pinMode(_pinMap[DI4], INPUT);

  pinMode(_pinMap[DI5], INPUT);
  pinMode(_pinMap[DI6], INPUT);

#ifdef IONO_RP
  gpio_disable_pulls(IONO_PIN_DI1);
  gpio_disable_pulls(IONO_PIN_DI2);
  gpio_disable_pulls(IONO_PIN_DI3);
  gpio_disable_pulls(IONO_PIN_DI4);
  gpio_disable_pulls(IONO_PIN_DI5);
  gpio_disable_pulls(IONO_PIN_DI6);
  gpio_disable_pulls(IONO_PIN_DI5_BYP);
  gpio_disable_pulls(IONO_PIN_DI6_BYP);
#endif

#ifdef PIN_TXEN
  pinMode(PIN_TXEN, OUTPUT);
#endif
#ifdef PIN_TXEN_N
  pinMode(PIN_TXEN_N, OUTPUT);
#endif

  serialTxEn(false);

#ifdef IONO_RP
  IONO_RS485.setRX(17);
  IONO_RS485.setTX(16);
  // Required for TX LED off
  IONO_RS485.begin(9600);
#endif

#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
  // For Arduino UNO, Ethernet, Leonardo ETH and UNO WIFI REV2 to use the external 3.3V reference
  analogReference(EXTERNAL);
#elif defined(IONO_RP)
  analogReadResolution(ANALOG_READ_BITS);
  analogWriteResolution(ANALOG_WRITE_BITS);
  analogWriteFreq(1000);
#else
  analogReadResolution(ANALOG_READ_BITS);
  analogWriteResolution(ANALOG_WRITE_BITS);
#endif

  write(AO1, 0);
}

#if defined(IONO_MKR) || defined(IONO_RP)
void IonoClass::setBYP(uint8_t pin, bool value) {
  if (pin == DI5) {
    _pinMap[DI5] = value ? IONO_PIN_DI5_BYP : IONO_PIN_DI5;
    pinMode(_pinMap[DI5], INPUT);
  } else if (pin == DI6) {
    _pinMap[DI6] = value ? IONO_PIN_DI6_BYP : IONO_PIN_DI6;
    pinMode(_pinMap[DI6], INPUT);
  }
}
#endif

void IonoClass::setBypass(uint8_t pin, iono_pin_mode_t mode) {
  if (pin == DI5) {
    _pinMap[DI5] = IONO_PIN_DI5_BYP;
    pinMode(_pinMap[DI5], mode);
  } else if (pin == DI6) {
    _pinMap[DI6] = IONO_PIN_DI6_BYP;
    pinMode(_pinMap[DI6], mode);
  }
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
  (*input).value = -1;
  (*input).lastTS = millis();
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

    case AO1:
      input = &_a1;
      break;

    default:
      return;
  }

  (*input).pin = pin;
  (*input).stableTime = stableTime;
  (*input).minVariation = minVariation;
  (*input).callback = callback;
  (*input).value = -100;
  (*input).lastTS = millis();
}

void IonoClass::linkDiDo(uint8_t dix, uint8_t dox, uint8_t mode, unsigned long stableTime) {
  if (dox < DO1 || dox > DO6) {
    return;
  }

  CallbackMap* input;

  switch (dix) {
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

    default:
      return;
  }

  (*input).pin = dix;
  (*input).stableTime = stableTime;
  (*input).minVariation = 0;
  (*input).linkedPin = dox;
  (*input).linkMode = mode;
  (*input).value = -1;
  (*input).lastTS = millis();
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
  check(&_a1);
}

void IonoClass::check(CallbackMap *input) {
  if ((*input).callback != NULL || (*input).linkedPin >= 0) {
    float val = read((*input).pin);
    unsigned long ts = millis();

    if ((*input).value != val) {
      float diff = (*input).value - val;
      diff = abs(diff);

      float maxVal;
      if ((*input).pin == AV1 || (*input).pin == AV2 || (*input).pin == AV3 || (*input).pin == AV4) {
        maxVal = IONO_AV_MAX;
      } else if ((*input).pin == AI1 || (*input).pin == AI2 || (*input).pin == AI3 || (*input).pin == AI4) {
        maxVal = IONO_AI_MAX;
      } else if ((*input).pin == AO1) {
        maxVal = IONO_AO_MAX;
      } else {
        maxVal = -1;
      }

      if (diff >= (*input).minVariation || val == 0 || val == maxVal) {
        if ((ts - (*input).lastTS) >= (*input).stableTime) {
          (*input).value = val;
          (*input).lastTS = ts;
          if ((*input).callback != NULL) {
            (*input).callback((*input).pin, val);
          }
          if ((*input).linkedPin >= 0) {
            switch ((*input).linkMode) {
              case LINK_FOLLOW:
                write((*input).linkedPin, val);
                break;
              case LINK_INVERT:
                write((*input).linkedPin, val == HIGH ? LOW : HIGH);
                break;
              case LINK_FLIP_T:
                flip((*input).linkedPin);
                break;
              case LINK_FLIP_H:
                if (val == HIGH) {
                  flip((*input).linkedPin);
                }
                break;
              case LINK_FLIP_L:
                if (val == LOW) {
                  flip((*input).linkedPin);
                }
                break;
            }
          }
        }
      } else {
        (*input).lastTS = ts;
      }
    } else {
      (*input).lastTS = ts;
    }
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
    return analogRead(_pinMap[pin]) * IONO_AV_MAX / ANALOG_READ_MAX;
  }

  if (pin == AI1 || pin == AI2 || pin == AI3 || pin == AI4) {
    return analogRead(_pinMap[pin]) * IONO_AI_MAX / ANALOG_READ_MAX;
  }

  if (pin == AO1) {
    return _ao1_val;
  }

  return -1;
}

float IonoClass::readAnalogAvg(uint8_t pin, int n) {
  int nn = n;
  unsigned long sum = 0;

  if (pin == AV1 || pin == AV2 || pin == AV3 || pin == AV4) {
    pin = _pinMap[pin];
    for (; nn > 0; nn--) {
      sum += analogRead(pin);
    }
    return sum / n * IONO_AV_MAX / ANALOG_READ_MAX;
  }

  if (pin == AI1 || pin == AI2 || pin == AI3 || pin == AI4) {
    pin = _pinMap[pin];
    for (; nn > 0; nn--) {
      sum += analogRead(pin);
    }
    return sum / n * IONO_AI_MAX / ANALOG_READ_MAX;
  }

  return -1;
}

void IonoClass::write(uint8_t pin, float value) {
  if (pin >= DO1 && pin <= DO6) {
    digitalWrite(_pinMap[pin], (int) value);
  }

  else if (pin == AO1) {
    if (value < 0) {
      value = 0;
    } else if (value > IONO_AO_MAX) {
      value = IONO_AO_MAX;
    }
    analogWrite(_pinMap[pin], value * ANALOG_WRITE_MAX / IONO_AO_MAX);
    _ao1_val = value;
  }
}

void IonoClass::flip(uint8_t pin) {
  write(pin, read(pin) == HIGH ? LOW : HIGH);
}

void IonoClass::serialTxEn(bool enabled) {
#ifdef PIN_TXEN
  digitalWrite(PIN_TXEN, enabled ? HIGH : LOW);
#endif
#ifdef PIN_TXEN_N
  digitalWrite(PIN_TXEN_N, enabled ? LOW : HIGH);
#endif
}

IonoClass Iono;
