/*
  IonoHQ.h - Arduino library for Iono MKR's seismic sensor

    Copyright (C) 2018 Sfera Labs S.r.l. - All rights reserved.

    For information, see the iono web site:
    http://www.sferalabs.cc/iono-arduino

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#ifndef IonoHQ_h
#define IonoHQ_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <Wire.h>

class IonoHQClass
{
  public:
    void begin();
    void attachShutoffISR(void (*ISR)(void));
    void detachShutoffISR();
    void attachProcessingISR(void (*ISR)(void));
    void detachProcessingISR();

    int32_t readD7S(byte addrH, byte addrL, int n);
    byte writeD7S(byte addrH, byte addrL, byte data);
    int32_t adjust(int32_t val);

    // Settings
    int16_t readState();
    int16_t readAxisState();
    int16_t readEvent();
    int16_t readMode();
    boolean writeMode(byte data);
    int16_t readCtrl();
    boolean writeCtrl(byte data);
    int16_t readClearCommand();
    boolean writeClearCommand(byte data);

    // Current
    int32_t readCurrentSI();
    int32_t readCurrentPGA();

    // Latest
    int32_t readLatestOffsetX(int n);
    int32_t readLatestOffsetY(int n);
    int32_t readLatestOffsetZ(int n);
    int32_t readLatestTemp(int n);
    int32_t readLatestSI(int n);
    int32_t readLatestPGA(int n);

    // SI Ranked
    int32_t readRankedOffsetX(int n);
    int32_t readRankedOffsetY(int n);
    int32_t readRankedOffsetZ(int n);
    int32_t readRankedTemp(int n);
    int32_t readRankedSI(int n);
    int32_t readRankedPGA(int n);
};

extern IonoHQClass IonoHQ;

#endif
