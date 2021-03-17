/*
  IonoModbusRtuSlave.cpp - Modbus RTU Slave library for Iono Arduino and Iono MKR

    Copyright (C) 2018-2021 Sfera Labs S.r.l. - All rights reserved.

    For information, see:
    https://www.sferalabs.cc/

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#include "IonoModbusRtuSlave.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#ifdef IONO_MKR
#define DO_MAX_INDEX 4
#else
#define DO_MAX_INDEX 6
#endif

#define ANALOG_AVG_N 32
#define ONE_WIRE_BUS_DI5 0
#define ONE_WIRE_BUS_DI6 1
#define ONE_WIRE_REQ_ITVL 10000

bool IonoModbusRtuSlaveClass::_di1deb;
bool IonoModbusRtuSlaveClass::_di2deb;
bool IonoModbusRtuSlaveClass::_di3deb;
bool IonoModbusRtuSlaveClass::_di4deb;
bool IonoModbusRtuSlaveClass::_di5deb;
bool IonoModbusRtuSlaveClass::_di6deb;

word IonoModbusRtuSlaveClass::_di1count = 0;
word IonoModbusRtuSlaveClass::_di2count = 0;
word IonoModbusRtuSlaveClass::_di3count = 0;
word IonoModbusRtuSlaveClass::_di4count = 0;
word IonoModbusRtuSlaveClass::_di5count = 0;
word IonoModbusRtuSlaveClass::_di6count = 0;

IonoClass::Callback *IonoModbusRtuSlaveClass::_di1Callback = NULL;
IonoClass::Callback *IonoModbusRtuSlaveClass::_di2Callback = NULL;
IonoClass::Callback *IonoModbusRtuSlaveClass::_di3Callback = NULL;
IonoClass::Callback *IonoModbusRtuSlaveClass::_di4Callback = NULL;
IonoClass::Callback *IonoModbusRtuSlaveClass::_di5Callback = NULL;
IonoClass::Callback *IonoModbusRtuSlaveClass::_di6Callback = NULL;

char IonoModbusRtuSlaveClass::_inMode[4] = {0, 0, 0, 0};

ModbusRtuSlaveClass::Callback *IonoModbusRtuSlaveClass::_customCallback = NULL;

OneWire oneWireDi5(ONE_WIRE_BUS_DI5);
OneWire oneWireDi6(ONE_WIRE_BUS_DI6);
DallasTemperature sensorsDi5(&oneWireDi5);
DallasTemperature sensorsDi6(&oneWireDi6);
DeviceAddress sensorsAddressDi5[8];
DeviceAddress sensorsAddressDi6[8];
int sensorsCountDi5 = -1;
int sensorsCountDi6 = -1;
long sensorsReqTsDi5;
long sensorsReqTsDi6;

void IonoModbusRtuSlaveClass::begin(byte unitAddr, unsigned long baud, unsigned long config, unsigned long diDebounceTime) {
  SERIAL_PORT_HARDWARE.begin(baud, config);
  ModbusRtuSlave.setCallback(&IonoModbusRtuSlaveClass::onRequest);

#ifdef PIN_TXEN
  ModbusRtuSlave.begin(unitAddr, &SERIAL_PORT_HARDWARE, baud, PIN_TXEN);
#else
  ModbusRtuSlave.begin(unitAddr, &SERIAL_PORT_HARDWARE, baud, 0);
#endif

  Iono.setup();

  if (_inMode[0] == 0 || _inMode[0] == 'D') {
    Iono.subscribeDigital(DI1, diDebounceTime, &onDIChange);
  }
  if (_inMode[1] == 0 || _inMode[1] == 'D') {
    Iono.subscribeDigital(DI2, diDebounceTime, &onDIChange);
  }
  if (_inMode[2] == 0 || _inMode[2] == 'D') {
    Iono.subscribeDigital(DI3, diDebounceTime, &onDIChange);
  }
  if (_inMode[3] == 0 || _inMode[3] == 'D') {
    Iono.subscribeDigital(DI4, diDebounceTime, &onDIChange);
  }
  Iono.subscribeDigital(DI5, diDebounceTime, &onDIChange);
  Iono.subscribeDigital(DI6, diDebounceTime, &onDIChange);
}

void IonoModbusRtuSlaveClass::setInputMode(int idx, char mode) {
  if (idx >= 1 && idx <= 4 &&
      (mode == 0 || mode == 'D' || mode == 'V' || mode == 'I')) {
    _inMode[idx - 1] = mode;
  }
}

void IonoModbusRtuSlaveClass::process() {
  ModbusRtuSlave.process();
  Iono.process();
  if (sensorsCountDi5 > 0 && millis() - sensorsReqTsDi5 > ONE_WIRE_REQ_ITVL) {
    sensorsDi5.requestTemperatures();
    sensorsReqTsDi5 = millis();
  }
  if (sensorsCountDi6 > 0 && millis() - sensorsReqTsDi6 > ONE_WIRE_REQ_ITVL) {
    sensorsDi6.requestTemperatures();
    sensorsReqTsDi6 = millis();
  }
}

void IonoModbusRtuSlaveClass::setCustomHandler(ModbusRtuSlaveClass::Callback *callback) {
  _customCallback = callback;
}

void IonoModbusRtuSlaveClass::subscribeDigital(uint8_t pin, IonoClass::Callback *callback) {
  switch (pin) {
    case DI1:
      _di1Callback = callback;
      break;

    case DI2:
      _di2Callback = callback;
      break;

    case DI3:
      _di3Callback = callback;
      break;

    case DI4:
      _di4Callback = callback;
      break;

    case DI5:
      _di5Callback = callback;
      break;

    case DI6:
      _di6Callback = callback;
      break;
  }
}

void IonoModbusRtuSlaveClass::onDIChange(uint8_t pin, float value) {
  switch (pin) {
    case DI1:
      _di1deb = value == HIGH;
      if (_di1deb) {
        _di1count++;
      }
      if (_di1Callback != NULL) {
        _di1Callback(pin, value);
      }
      break;

    case DI2:
      _di2deb = value == HIGH;
      if (_di2deb) {
        _di2count++;
      }
      if (_di2Callback != NULL) {
        _di2Callback(pin, value);
      }
      break;

    case DI3:
      _di3deb = value == HIGH;
      if (_di3deb) {
        _di3count++;
      }
      if (_di3Callback != NULL) {
        _di3Callback(pin, value);
      }
      break;

    case DI4:
      _di4deb = value == HIGH;
      if (_di4deb) {
        _di4count++;
      }
      if (_di4Callback != NULL) {
        _di4Callback(pin, value);
      }
      break;

    case DI5:
      _di5deb = value == HIGH;
      if (_di5deb) {
        _di5count++;
      }
      if (_di5Callback != NULL) {
        _di5Callback(pin, value);
      }
      break;

    case DI6:
      _di6deb = value == HIGH;
      if (_di6deb) {
        _di6count++;
      }
      if (_di6Callback != NULL) {
        _di6Callback(pin, value);
      }
      break;
  }
}

byte IonoModbusRtuSlaveClass::onRequest(byte unitAddr, byte function, word regAddr, word qty, byte *data) {
  byte respCode;
  if (_customCallback != NULL) {
    respCode = _customCallback(unitAddr, function, regAddr, qty, data);
    if (respCode != MB_RESP_PASS) {
      return respCode;
    }
  }

  switch (function) {
    case MB_FC_READ_COILS:
      if (checkAddrRange(regAddr, qty, 1, DO_MAX_INDEX)) {
        for (int i = regAddr; i < regAddr + qty; i++) {
          ModbusRtuSlave.responseAddBit(Iono.read(indexToDO(i)) == HIGH);
        }
        return MB_RESP_OK;
      }
      return MB_EX_ILLEGAL_DATA_ADDRESS;

    case MB_FC_READ_DISCRETE_INPUTS:
      if (checkAddrRange(regAddr, qty, 101, 106)) {
        for (int i = regAddr - 100; i < regAddr - 100 + qty; i++) {
          ModbusRtuSlave.responseAddBit(indexToDIdeb(i));
        }
        return MB_RESP_OK;
      }
      if (checkAddrRange(regAddr, qty, 111, 116)) {
        for (int i = regAddr - 110; i < regAddr - 110 + qty; i++) {
          if (i > 4 || _inMode[i - 1] == 0 || _inMode[i - 1] == 'D') {
            ModbusRtuSlave.responseAddBit(Iono.read(indexToDI(i)) == HIGH);
          } else {
            ModbusRtuSlave.responseAddBit(false);
          }
        }
        return MB_RESP_OK;
      }
      return MB_EX_ILLEGAL_DATA_ADDRESS;

    case MB_FC_READ_HOLDING_REGISTERS:
      if (regAddr == 601 && qty == 1) {
        ModbusRtuSlave.responseAddRegister(Iono.read(AO1) * 1000);
        return MB_RESP_OK;
      }
      if (regAddr == 5000 && qty == 1) {
        sensorsDi5.begin();
        sensorsCountDi5 = sensorsDi5.getDeviceCount();
        for (int i = 0; i < sensorsCountDi5; i++) {
          sensorsDi5.getAddress(sensorsAddressDi5[i], i);
        }
        sensorsDi5.setWaitForConversion(false);
        sensorsDi5.requestTemperatures();
        sensorsReqTsDi5 = millis();
        ModbusRtuSlave.responseAddRegister(sensorsCountDi5);
        return MB_RESP_OK;
      }
      if (checkAddrRange(regAddr, qty, 5001, 5064)) {
        for (int i = regAddr - 5001; i < regAddr - 5001 + qty; i++) {
          int a = i / 8;
          int b = i % 8;
          if (a < sensorsCountDi5) {
            ModbusRtuSlave.responseAddRegister(sensorsAddressDi5[a][b]);
          } else {
            ModbusRtuSlave.responseAddRegister(0);
          }
        }
        return MB_RESP_OK;
      }
      if (regAddr == 6000 && qty == 1) {
        sensorsDi6.begin();
        sensorsCountDi6 = sensorsDi6.getDeviceCount();
        for (int i = 0; i < sensorsCountDi6; i++) {
          sensorsDi6.getAddress(sensorsAddressDi6[i], i);
        }
        sensorsDi6.setWaitForConversion(false);
        sensorsDi6.requestTemperatures();
        sensorsReqTsDi6 = millis();
        ModbusRtuSlave.responseAddRegister(sensorsCountDi6);
        return MB_RESP_OK;
      }
      if (checkAddrRange(regAddr, qty, 6001, 6064)) {
        for (int i = regAddr - 6001; i < regAddr - 6001 + qty; i++) {
          int a = i / 8;
          int b = i % 8;
          if (a < sensorsCountDi6) {
            ModbusRtuSlave.responseAddRegister(sensorsAddressDi6[a][b]);
          } else {
            ModbusRtuSlave.responseAddRegister(0);
          }
        }
        return MB_RESP_OK;
      }
      return MB_EX_ILLEGAL_DATA_ADDRESS;

    case MB_FC_READ_INPUT_REGISTER:
      if (checkAddrRange(regAddr, qty, 201, 204)) {
        for (int i = regAddr - 200; i < regAddr - 200 + qty; i++) {
          if (_inMode[i - 1] != 'D') {
            ModbusRtuSlave.responseAddRegister(Iono.read(indexToAV(i)) * 1000);
          } else {
            ModbusRtuSlave.responseAddRegister(0);
          }
        }
        return MB_RESP_OK;
      }
      if (checkAddrRange(regAddr, qty, 211, 214)) {
        for (int i = regAddr - 210; i < regAddr - 210 + qty; i++) {
          if (_inMode[i - 1] != 'D') {
            ModbusRtuSlave.responseAddRegister(Iono.readAnalogAvg(indexToAV(i), ANALOG_AVG_N) * 1000);
          } else {
            ModbusRtuSlave.responseAddRegister(0);
          }
        }
        return MB_RESP_OK;
      }
      if (checkAddrRange(regAddr, qty, 301, 304)) {
        for (int i = regAddr - 300; i < regAddr - 300 + qty; i++) {
          if (_inMode[i - 1] != 'D') {
            ModbusRtuSlave.responseAddRegister(Iono.read(indexToAI(i)) * 1000);
          } else {
            ModbusRtuSlave.responseAddRegister(0);
          }
        }
        return MB_RESP_OK;
      }
      if (checkAddrRange(regAddr, qty, 311, 314)) {
        for (int i = regAddr - 310; i < regAddr - 310 + qty; i++) {
          if (_inMode[i - 1] != 'D') {
            ModbusRtuSlave.responseAddRegister(Iono.readAnalogAvg(indexToAI(i), ANALOG_AVG_N) * 1000);
          } else {
            ModbusRtuSlave.responseAddRegister(0);
          }
        }
        return MB_RESP_OK;
      }
      if (checkAddrRange(regAddr, qty, 1001, 1006)) {
        for (int i = regAddr - 1000; i < regAddr - 1000 + qty; i++) {
          ModbusRtuSlave.responseAddRegister(indexToDIcount(i));
        }
        return MB_RESP_OK;
      }
      if (checkAddrRange(regAddr, qty, 5101, 5108)) {
        if (qty <= sensorsCountDi5) {
          for (int i = regAddr - 5101; i < regAddr - 5101 + qty; i++) {
            ModbusRtuSlave.responseAddRegister((int)(sensorsDi5.getTempC(sensorsAddressDi5[i]) * 100.0));
          }
          return MB_RESP_OK;
        }
      }
      if (checkAddrRange(regAddr, qty, 6101, 6108)) {
        if (qty <= sensorsCountDi6) {
          sensorsDi6.requestTemperatures();
          for (int i = regAddr - 6101; i < regAddr - 6101 + qty; i++) {
            ModbusRtuSlave.responseAddRegister((int)(sensorsDi6.getTempC(sensorsAddressDi6[i]) * 100.0));
          }
          return MB_RESP_OK;
        }
      }
      if (regAddr == 99 && qty == 1) {
#ifdef IONO_MKR
        ModbusRtuSlave.responseAddRegister(0x20);
#else
        ModbusRtuSlave.responseAddRegister(0x10);
#endif
        return MB_RESP_OK;
      }
      if (regAddr == 64990 && qty == 4) {
        ModbusRtuSlave.responseAddRegister(0xCAFE); // fixed
        ModbusRtuSlave.responseAddRegister(0xBEAF); // fixed
#ifdef IONO_MKR
        ModbusRtuSlave.responseAddRegister(0x2001); // Iono MKR - App ID: Modbus RTU
#else
        ModbusRtuSlave.responseAddRegister(0x1001); // Iono Arduino - App ID: Modbus RTU
#endif
        ModbusRtuSlave.responseAddRegister(0x0400); // Version High - Version Low
        return MB_RESP_OK;
      }
      return MB_EX_ILLEGAL_DATA_ADDRESS;

    case MB_FC_WRITE_SINGLE_COIL:
      if (regAddr >= 1 && regAddr <= DO_MAX_INDEX) {
        bool on = ModbusRtuSlave.getDataCoil(function, data, 0);
        Iono.write(indexToDO(regAddr), on ? HIGH : LOW);
        return MB_RESP_OK;
      }
      return MB_EX_ILLEGAL_DATA_ADDRESS;

    case MB_FC_WRITE_SINGLE_REGISTER:
      if (regAddr == 601) {
        word value = ModbusRtuSlave.getDataRegister(function, data, 0);
        if (value < 0 || value > 10000) {
          return MB_EX_ILLEGAL_DATA_VALUE;
        }
        Iono.write(AO1, value / 1000.0);
        return MB_RESP_OK;
      }
      return MB_EX_ILLEGAL_DATA_ADDRESS;

    case MB_FC_WRITE_MULTIPLE_COILS:
      if (checkAddrRange(regAddr, qty, 1, DO_MAX_INDEX)) {
        for (int i = regAddr; i < regAddr + qty; i++) {
          bool on = ModbusRtuSlave.getDataCoil(function, data, i - regAddr);
          Iono.write(indexToDO(i), on ? HIGH : LOW);
        }
        return MB_RESP_OK;
      }
      return MB_EX_ILLEGAL_DATA_ADDRESS;

    default:
      return MB_EX_ILLEGAL_FUNCTION;
  }
}

bool IonoModbusRtuSlaveClass::checkAddrRange(word regAddr, word qty, word min, word max) {
  return regAddr >= min && regAddr <= max && regAddr + qty <= max + 1;
}

uint8_t IonoModbusRtuSlaveClass::indexToDO(int i) {
  switch (i) {
    case 1:
      return DO1;
    case 2:
      return DO2;
    case 3:
      return DO3;
    case 4:
      return DO4;
    case 5:
      return DO5;
    case 6:
      return DO6;
  }
}

uint8_t IonoModbusRtuSlaveClass::indexToDI(int i) {
  switch (i) {
    case 1:
      return DI1;
    case 2:
      return DI2;
    case 3:
      return DI3;
    case 4:
      return DI4;
    case 5:
      return DI5;
    case 6:
      return DI6;
  }
}

bool IonoModbusRtuSlaveClass::indexToDIdeb(int i) {
  switch (i) {
    case 1:
      return _di1deb;
    case 2:
      return _di2deb;
    case 3:
      return _di3deb;
    case 4:
      return _di4deb;
    case 5:
      return _di5deb;
    case 6:
      return _di6deb;
  }
}

word IonoModbusRtuSlaveClass::indexToDIcount(int i) {
  switch (i) {
    case 1:
      return _di1count;
    case 2:
      return _di2count;
    case 3:
      return _di3count;
    case 4:
      return _di4count;
    case 5:
      return _di5count;
    case 6:
      return _di6count;
  }
}

uint8_t IonoModbusRtuSlaveClass::indexToAV(int i) {
  switch (i) {
    case 1:
      return AV1;
    case 2:
      return AV2;
    case 3:
      return AV3;
    case 4:
      return AV4;
  }
}

uint8_t IonoModbusRtuSlaveClass::indexToAI(int i) {
  switch (i) {
    case 1:
      return AI1;
    case 2:
      return AI2;
    case 3:
      return AI3;
    case 4:
      return AI4;
  }
}
