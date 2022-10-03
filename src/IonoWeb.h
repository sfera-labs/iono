/*
  IonoWeb.h - Arduino library for the control of Iono Uno Ethernet via HTTP API

    Copyright (C) 2014-2022 Sfera Labs S.r.l. - All rights reserved.

    For information, see:
    https://www.sferalabs.cc/

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#ifndef IonoWeb_h
#define IonoWeb_h

#include <Iono.h>
#include "WebServer.h"

#define SUBSCRIBE_TIMEOUT 60000

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
    static unsigned long _lastSubscribeTime;

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
