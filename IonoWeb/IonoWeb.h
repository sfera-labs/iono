/*
  IonoWeb.h - Arduino library for the control of iono ethernet via a HTTP-based API

    Copyright (C) 2014-2015 Sfera Labs, a division of Home Systems Consulting S.p.A. - All rights reserved.

    For information, see the iono web site:
    http://www.iono.cc/
  
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#ifndef IonoWeb_h
#define IonoWeb_h

#include <SPI.h>
#include <Ethernet.h>
#include <Iono.h>
#define WEBDUINO_NO_IMPLEMENTATION true
#include "WebServer.h"

class IonoWebClass
{
  public:
    static void begin(int port);
    static void processRequest();
    static void subscribe(unsigned long stableTime, float minVariation, char *host, int port, char *command, uint8_t mode1, uint8_t mode2, uint8_t mode3, uint8_t mode4);
    static WebServer& getWebServer();

  private:
    static WebServer _webServer;

    static char *_host;
    static int _port;
    static char *_command;

    static void setCommand(WebServer &webServer, WebServer::ConnectionType type, char* urlTail, bool tailComplete);
    static void jsonStateCommand(WebServer &webServer, WebServer::ConnectionType type, char* urlTail, bool tailComplete);
    static void subscribeCommand(WebServer &webServer, WebServer::ConnectionType type, char* urlTail, bool tailComplete);
    static void callDigitalURL(uint8_t pin, float value);
    static void callAnalogURL(uint8_t pin, float value);
    static void callURL(const char *pin, const char *value);
    static void ftoa(char *sVal, float fVal);
};

extern IonoWebClass IonoWeb;

#endif
