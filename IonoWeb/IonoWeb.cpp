/*
  IonoWeb.cpp - Arduino library for the control of iono ethernet via a HTTP-based API

    Copyright (C) 2014-2017 Sfera Labs S.r.l. - All rights reserved.

    For information, see the iono web site:
    http://www.sferalabs.cc/iono-arduino

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#include "IonoWeb.h"

WebServer IonoWebClass::_webServer;

char *IonoWebClass::_host = (char*) malloc(32);
int IonoWebClass::_port = 0;
char *IonoWebClass::_command = (char*) malloc(32);
unsigned long IonoWebClass::_lastSubscribeTime = 0;

void IonoWebClass::begin(int port) {
  _webServer = WebServer("", port);
  _webServer.addCommand("api/state", &IonoWebClass::jsonStateCommand);
  _webServer.addCommand("api/set", &IonoWebClass::setCommand);
  _webServer.addCommand("api/subscribe", &IonoWebClass::subscribeCommand);
  _webServer.begin();
}

void IonoWebClass::processRequest() {
  int len = 128;
  char buff[len];
  _webServer.processConnection(buff, &len);
}

WebServer& IonoWebClass::getWebServer() {
  return _webServer;
}

void IonoWebClass::jsonStateCommand(WebServer &webServer, WebServer::ConnectionType type, char* urlTail, bool tailComplete) {
  webServer.httpSuccess("application/json");
  webServer.print("{");

    webServer.print("\"DO1\":");
    webServer.print((int) Iono.read(DO1));
    webServer.print(",");

    webServer.print("\"DO2\":");
    webServer.print((int) Iono.read(DO2));
    webServer.print(",");

    webServer.print("\"DO3\":");
    webServer.print((int) Iono.read(DO3));
    webServer.print(",");

    webServer.print("\"DO4\":");
    webServer.print((int) Iono.read(DO4));
    webServer.print(",");

    webServer.print("\"DO5\":");
    webServer.print((int) Iono.read(DO5));
    webServer.print(",");

    webServer.print("\"DO6\":");
    webServer.print((int) Iono.read(DO6));
    webServer.print(",");

    webServer.print("\"I1\":{");
      webServer.print("\"D\":");
      webServer.print((int) Iono.read(DI1));
      webServer.print(",");

      webServer.print("\"V\":");
      webServer.print(Iono.read(AV1));
      webServer.print(",");

      webServer.print("\"I\":");
      webServer.print(Iono.read(AI1));
    webServer.print("},");

    webServer.print("\"I2\":{");
      webServer.print("\"D\":");
      webServer.print((int) Iono.read(DI2));
      webServer.print(",");

      webServer.print("\"V\":");
      webServer.print(Iono.read(AV2));
      webServer.print(",");

      webServer.print("\"I\":");
      webServer.print(Iono.read(AI2));
    webServer.print("},");

    webServer.print("\"I3\":{");
      webServer.print("\"D\":");
      webServer.print((int) Iono.read(DI3));
      webServer.print(",");

      webServer.print("\"V\":");
      webServer.print(Iono.read(AV3));
      webServer.print(",");

      webServer.print("\"I\":");
      webServer.print(Iono.read(AI3));
    webServer.print("},");

    webServer.print("\"I4\":{");
      webServer.print("\"D\":");
      webServer.print((int) Iono.read(DI4));
      webServer.print(",");

      webServer.print("\"V\":");
      webServer.print(Iono.read(AV4));
      webServer.print(",");

      webServer.print("\"I\":");
      webServer.print(Iono.read(AI4));
    webServer.print("},");

    webServer.print("\"I5\":{");
      webServer.print("\"D\":");
      webServer.print((int) Iono.read(DI5));
    webServer.print("},");

    webServer.print("\"I6\":{");
      webServer.print("\"D\":");
      webServer.print((int) Iono.read(DI6));
    webServer.print("}");

  webServer.print("}");
}

void IonoWebClass::setCommand(WebServer &webServer, WebServer::ConnectionType type, char* urlTail, bool tailComplete) {
  if (!tailComplete) {
    webServer.httpFail();
    return;
  }

  char name[8];
  char value[8];
  URLPARAM_RESULT rc;

  while (strlen(urlTail)) {
    rc = webServer.nextURLparam(&urlTail, name, 8, value, 8);
    if (rc == URLPARAM_EOS) {
       webServer.httpFail();
       return;
    }

    if (strlen(name) == 3) {
      if (name[0] == 'D' && name[1] == 'O') {
        uint8_t pin;
        switch (name[2]) {
          case '1':
            pin = DO1;
            break;
          case '2':
            pin = DO2;
            break;
          case '3':
            pin = DO3;
            break;
          case '4':
            pin = DO4;
            break;
          case '5':
            pin = DO5;
            break;
          case '6':
            pin = DO6;
            break;
          default:
            webServer.httpFail();
            return;
        }

        if (value[0] == 'f') {
          Iono.flip(pin);
        } else {
          Iono.write(pin, value[0] == '1' ? HIGH : LOW);
        }

      } else if (name[0] == 'A' && name[1] == 'O' && name[2] == '1') {
        Iono.write(AO1, atof(value));
      }
    }
  }

  webServer.httpSuccess();
}

void IonoWebClass::subscribeCommand(WebServer &webServer, WebServer::ConnectionType type, char* urlTail, bool tailComplete) {
  if (!tailComplete) {
    webServer.httpFail();
    return;
  }

  unsigned long stableTime = 0;
  float minVariation = 0;

  char host[32];
  int port = 80;
  char command[32];
  uint8_t mode1 = 1;
  uint8_t mode2 = 1;
  uint8_t mode3 = 1;
  uint8_t mode4 = 1;

  char name[8];
  char value[32];
  URLPARAM_RESULT rc;

  while (strlen(urlTail)) {
    rc = webServer.nextURLparam(&urlTail, name, 8, value, 32);
    if (rc == URLPARAM_EOS) {
      webServer.httpFail();
      return;
    }

    if (strcmp(name, "st") == 0) {
      stableTime = atol(value);

    } else if (strcmp(name, "mv") == 0) {
      minVariation = atof(value);

    } else if (strcmp(name, "host") == 0) {
      strncpy(host, value, 32);

    } else if (strcmp(name, "port") == 0) {
      port = atoi(value);

    } else if (strcmp(name, "cmd") == 0) {
      strncpy(command, value, 32);

    } else if (strcmp(name, "mode1") == 0) {
      switch (value[0]) {
        case 'i':
          mode1 = 3;
          break;

        case 'v':
          mode1 = 2;
          break;
      }

    } else if (strcmp(name, "mode2") == 0) {
      switch (value[0]) {
        case 'i':
          mode2 = 3;
          break;

        case 'v':
          mode2 = 2;
          break;
      }

    } else if (strcmp(name, "mode3") == 0) {
      switch (value[0]) {
        case 'i':
          mode3 = 3;
          break;

        case 'v':
          mode3 = 2;
          break;
      }

    } else if (strcmp(name, "mode4") == 0) {
      switch (value[0]) {
        case 'i':
          mode4 = 3;
          break;

        case 'v':
          mode4 = 2;
          break;
      }
    }
  }

  subscribe(stableTime, minVariation, host, port, command, mode1, mode2, mode3, mode4);
  jsonStateCommand(webServer, type, urlTail, tailComplete);
}

void IonoWebClass::subscribe(unsigned long stableTime, float minVariation, char *host, int port, char *command, uint8_t mode1, uint8_t mode2, uint8_t mode3, uint8_t mode4) {
  strncpy(_host, host, 32);
  _port = port;
  strncpy(_command, command, 32);

  Iono.subscribeDigital(DO1, stableTime, &callDigitalURL);
  Iono.subscribeDigital(DO2, stableTime, &callDigitalURL);
  Iono.subscribeDigital(DO3, stableTime, &callDigitalURL);
  Iono.subscribeDigital(DO4, stableTime, &callDigitalURL);
  Iono.subscribeDigital(DO5, stableTime, &callDigitalURL);
  Iono.subscribeDigital(DO6, stableTime, &callDigitalURL);

  switch (mode1) {
    case 3:
      Iono.subscribeAnalog(AI1, stableTime, minVariation, &callAnalogURL);
      break;

    case 2:
      Iono.subscribeAnalog(AV1, stableTime, minVariation, &callAnalogURL);
      break;

    default:
      Iono.subscribeDigital(DI1, stableTime, &callDigitalURL);
  }

  switch (mode2) {
    case 3:
      Iono.subscribeAnalog(AI2, stableTime, minVariation, &callAnalogURL);
      break;

    case 2:
      Iono.subscribeAnalog(AV2, stableTime, minVariation, &callAnalogURL);
      break;

    default:
      Iono.subscribeDigital(DI2, stableTime, &callDigitalURL);
  }

  switch (mode3) {
    case 3:
      Iono.subscribeAnalog(AI3, stableTime, minVariation, &callAnalogURL);
      break;

    case 2:
      Iono.subscribeAnalog(AV3, stableTime, minVariation, &callAnalogURL);
      break;

    default:
      Iono.subscribeDigital(DI3, stableTime, &callDigitalURL);
  }

  switch (mode4) {
    case 3:
      Iono.subscribeAnalog(AI4, stableTime, minVariation, &callAnalogURL);
      break;

    case 2:
      Iono.subscribeAnalog(AV4, stableTime, minVariation, &callAnalogURL);
      break;

    default:
      Iono.subscribeDigital(DI4, stableTime, &callDigitalURL);
  }

  Iono.subscribeDigital(DI5, stableTime, &callDigitalURL);
  Iono.subscribeDigital(DI6, stableTime, &callDigitalURL);
  _lastSubscribeTime = millis();
}

void IonoWebClass::callDigitalURL(uint8_t pin, float value) {
  const char *v = value == HIGH ? "1" : "0";
  switch (pin) {
    case DI1:
      callURL("DI1", v);
      break;

    case DI2:
      callURL("DI2", v);
      break;

    case DI3:
      callURL("DI3", v);
      break;

    case DI4:
      callURL("DI4", v);
      break;

    case DI5:
      callURL("DI5", v);
      break;

    case DI6:
      callURL("DI6", v);
      break;

    case DO1:
      callURL("DO1", v);
      break;

    case DO2:
      callURL("DO2", v);
      break;

    case DO3:
      callURL("DO3", v);
      break;

    case DO4:
      callURL("DO4", v);
      break;

    case DO5:
      callURL("DO5", v);
      break;

    case DO6:
      callURL("DO6", v);
      break;
  }
}

void IonoWebClass::callAnalogURL(uint8_t pin, float value) {
  char sVal[6];
  ftoa(sVal, value);
  switch (pin) {
    case AV1:
      callURL("AV1", sVal);
      break;

    case AI1:
      callURL("AI1", sVal);
      break;

    case AV2:
      callURL("AV2", sVal);
      break;

    case AI2:
      callURL("AI2", sVal);
      break;

    case AV3:
      callURL("AV3", sVal);
      break;

    case AI3:
      callURL("AI3", sVal);
      break;

    case AV4:
      callURL("AV4", sVal);
      break;

    case AI4:
      callURL("AI4", sVal);
      break;
  }
}

void IonoWebClass::callURL(const char *pin, const char *value) {
  if (_port != 0) {
    if (millis() > _lastSubscribeTime + SUBSCRIBE_TIMEOUT) {
      _port = 0;
      return;
    }

    EthernetClient client;
    for (uint8_t i = 0; i < 4; i++) {
      if (client.connect(_host, _port)) {
        client.print("GET ");
        client.print(_command);
        client.print("?");
        client.print(pin);
        client.print("=");
        client.print(value);
        client.println(" HTTP/1.1");
        client.println("Connection: close");
        client.println();
        break;
      }
    }
    client.stop();
  }
}

void IonoWebClass::ftoa(char *sVal, float fVal) {
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

IonoWebClass IonoWeb;
