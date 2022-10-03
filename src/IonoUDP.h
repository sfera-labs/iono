/*
  IonoUDP.h - Arduino library for the control of Iono Uno Ethernet via a simple UDP protocol

    Copyright (C) 2014-2022 Sfera Labs S.r.l. - All rights reserved.

    For information, see:
    https://www.sferalabs.cc/

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#ifndef IonoUDP_h
#define IonoUDP_h

#include <SPI.h>
#include <Ethernet.h>
#include <Iono.h>

#define COMMAND_MAX_SIZE 10

class IonoUDPClass
{
  public:
    IonoUDPClass();
    void begin(const char *id, EthernetUDP Udp, unsigned int port, unsigned long stableTime, float minVariation);
    void process();

  private:
    static char _pinName[][4];

    IPAddress _ipBroadcast;
    float _lastValue[20];
    float _value[20];
    unsigned long _lastTS[20];
    char _progr;
    const char *_id;
    unsigned int _port;
    EthernetUDP _Udp;
    char _command[COMMAND_MAX_SIZE];
    unsigned long _stableTime;
    float _minVariation;
    unsigned long _lastSend;

    void checkState();
    void check(int pin);
    void send(int pin, float val);
    void ftoa(char *sVal, float fVal);
    void checkCommands();
};

extern IonoUDPClass IonoUDP;

#endif
