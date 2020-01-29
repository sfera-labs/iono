/*
  IonoUDP.cpp - Arduino library for the control of iono ethernet via a simple protocol employing UDP communication.

    Copyright (C) 2014-2016 Sfera Labs S.r.l. - All rights reserved.

    For information, see the iono web site:
    http://www.sferalabs.cc/iono

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#include "IonoUDP.h"

char IonoUDPClass::_pinName[][4] = {
  "DO1",
  "DO2",
  "DO3",
  "DO4",
  "DO5",
  "DO6",
  "DI1",
  "AV1",
  "AI1",
  "DI2",
  "AV2",
  "AI2",
  "DI3",
  "AV3",
  "AI3",
  "DI4",
  "AV4",
  "AI4",
  "DI5",
  "DI6",
  "AO1"
};

IonoUDPClass::IonoUDPClass() {
  _ipBroadcast = IPAddress(255, 255, 255, 255);

  for (int i = 0; i < 20; i++) {
    _lastValue[i] = -99;
    _value[i] = -99;
    _lastTS[i] = -99;
  }

  _progr = 0;
  _lastSend = 0;
}

void IonoUDPClass::begin(const char *id, EthernetUDP Udp, unsigned int port, unsigned long stableTime, float minVariation) {
  _id = id;
  _Udp = Udp;
  _port = port;
  _stableTime = stableTime;
  _minVariation = minVariation;
  Iono.setup();
}

void IonoUDPClass::process() {
  checkState();
  checkCommands();
}

void IonoUDPClass::checkState() {
  check(DO1);
  check(DO2);
  check(DO3);
  check(DO4);
  check(DO5);
  check(DO6);

  check(DI1);
  check(DI2);
  check(DI3);
  check(DI4);
  check(DI5);
  check(DI6);

  check(AV1);
  check(AV2);
  check(AV3);
  check(AV4);

  check(AI1);
  check(AI2);
  check(AI3);
  check(AI4);

  unsigned long ts = millis();
  if (ts - _lastSend > 30000) {
    for (int i = 0; i < 3; i++) {
      _Udp.beginPacket(_ipBroadcast, _port);
      _Udp.write("{\"id\":\"");
      _Udp.write(_id);
      _Udp.write("\",\"pr\":");
      _Udp.write('0' + _progr);
      _Udp.write("}");
      _Udp.endPacket();
      delay(3);
    }

    _progr = (_progr + 1) % 10;
    _lastSend = ts;
  }
}

void IonoUDPClass::check(int pin) {
  float val = Iono.read(pin);
  unsigned long ts = millis();

  if (val != _lastValue[pin]) {
    _lastTS[pin] = ts;
  }

  if ((ts - _lastTS[pin]) >= _stableTime) {
    if (val != _value[pin]) {
      float diff = _value[pin] - val;
      diff = abs(diff);
      if (diff >= _minVariation) {
        _value[pin] = val;
        send(pin, val);
        _lastSend = ts;
      }
    }
  }

  _lastValue[pin] = val;
}

void IonoUDPClass::send(int pin, float val) {
  char sVal[6];
  if (_pinName[pin][0] == 'D') {
    sVal[0] = val == HIGH ? '1' : '0';
    sVal[1] = '\0';
  } else {
    ftoa(sVal, val);
  }

  for (int i = 0; i < 3; i++) {
    _Udp.beginPacket(_ipBroadcast, _port);
    _Udp.write("{\"id\":\"");
    _Udp.write(_id);
    _Udp.write("\",\"pin\":\"");
    _Udp.write(_pinName[pin]);
    _Udp.write("\",\"pr\":");
    _Udp.write('0' + _progr);
    _Udp.write(",\"val\":");
    _Udp.write(sVal);
    _Udp.write("}");
    _Udp.endPacket();
    delay(3);
  }

  _progr = (_progr + 1) % 10;
}

void IonoUDPClass::ftoa(char *sVal, float fVal) {
  fVal += 0.005;

  int dVal = fVal;
  int dec = (int)(fVal * 100) % 100;

  int i = 0;
  int d = dVal / 10;
  if (d != 0) {
    sVal[i++] = d + '0';
  }
  sVal[i++] = (dVal % 10) + '0';
  sVal[i++] = '.';
  sVal[i++] = (dec / 10) + '0';
  sVal[i++] = (dec % 10) + '0';
  sVal[i] = '\0';
}

void IonoUDPClass::checkCommands() {
  int packetSize = _Udp.parsePacket();
  if(packetSize) {
    for (int i = 0; i < COMMAND_MAX_SIZE; i++) {
      _command[i] = '\0';
    }
    _Udp.read(_command, COMMAND_MAX_SIZE);
    if (strcmp(_command, "state") == 0) {
      _Udp.beginPacket(_Udp.remoteIP(), _Udp.remotePort());
      _Udp.write("{");
      _Udp.write("\"id\":\"");
      _Udp.write(_id);
      _Udp.write("\"");
      char sVal[6];
      for (int i = 0; i < 20; i++) {
        _Udp.write(",");
        _Udp.write("\"");
        _Udp.write(_pinName[i]);
        _Udp.write("\":");
        if (_pinName[i][0] == 'D') {
          _Udp.write(_value[i] == HIGH ? "1" : "0");
        } else {
          ftoa(sVal, _value[i]);
          _Udp.write(sVal);
        }
      }
      _Udp.write("}");
      _Udp.endPacket();
      return;

    } else if (strlen(_command) > 4 && _command[3] == '=') {
      char pn[4];
      for (int i = 0; i < 3; i++) {
        pn[i] = _command[i];
      }
      pn[3] = '\0';

      int pin = -1;
      for (int i = 0; i < 21; i++) {
        if (strcmp(_pinName[i], pn) == 0) {
          pin = i;
          break;
        }
      }

      if (pin != -1) {
        if (pn[0] == 'D') {
          if (_command[4] == 'f') {
            Iono.flip(pin);
          } else {
            Iono.write(pin, _command[4] == '1' ? HIGH : LOW);
          }

        } else {
          char val[6];
          for (int i = 0; i < 5; i++) {
            val[i] = _command[i + 4];
          }
          val[5] = '\0';
          Iono.write(pin, atof(val));
        }
      }

      _Udp.beginPacket(_Udp.remoteIP(), _Udp.remotePort());
      _Udp.write("ok");
      _Udp.endPacket();
      return;
    }

    _Udp.beginPacket(_Udp.remoteIP(), _Udp.remotePort());
    _Udp.write("error");
    _Udp.endPacket();
  }
}

IonoUDPClass IonoUDP;
