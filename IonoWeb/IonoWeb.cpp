/*
  IonoWeb.cpp
*/

#include "IonoWeb.h"

WebServer IonoWebClass::_webServer;

IonoWebClass::HttpURL IonoWebClass::_url1 = {"", 0, ""};
IonoWebClass::HttpURL IonoWebClass::_url2 = {"", 0, ""};
IonoWebClass::HttpURL IonoWebClass::_url3 = {"", 0, ""};
IonoWebClass::HttpURL IonoWebClass::_url4 = {"", 0, ""};
IonoWebClass::HttpURL IonoWebClass::_url5 = {"", 0, ""};
IonoWebClass::HttpURL IonoWebClass::_url6 = {"", 0, ""};

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
        int pin;
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
  
  char pin[4];
  unsigned long stableTime = 0;
  float minVariation = 0;

  char host[32];
  int port = 80;
  char command[32];

  char name[8];
  char value[32];
  URLPARAM_RESULT rc;
  
  while (strlen(urlTail)) {
    rc = webServer.nextURLparam(&urlTail, name, 8, value, 32);
    if (rc == URLPARAM_EOS) {
      webServer.httpFail();
      return;
    }

    if (strcmp(name, "pin") == 0) {
      strncpy(pin, value, 4);

    } else if (strcmp(name, "st") == 0) {
      stableTime = atol(value);

    } else if (strcmp(name, "mv") == 0) {
      minVariation = atof(value);

    } else if (strcmp(name, "host") == 0) {
      strncpy(host, value, 32);

    } else if (strcmp(name, "port") == 0) {
      port = atoi(value);

    } else if (strcmp(name, "cmd") == 0) {
      strncpy(command, value, 32);
    }
  }

  if (strlen(pin) == 3) {
    int pinNum = -1;
    if (pin[0] == 'D' && pin[1] == 'I') {
      switch (pin[2]) {
        case '1':
          pinNum = DI1;
          break;
        case '2':
          pinNum = DI2;
          break;
        case '3':
          pinNum = DI3;
          break;
        case '4':
          pinNum = DI4;
          break;
        case '5':
          pinNum = DI5;
          break;
        case '6':
          pinNum = DI6;
          break;
      }
      
    } else if (pin[0] == 'A') {
      if (pin[1] == 'V') {
        switch (pin[2]) {
          case '1':
            pinNum = AV1;
            break;
          case '2':
            pinNum = AV2;
            break;
          case '3':
            pinNum = AV3;
            break;
          case '4':
            pinNum = AV4;
            break;
        }
      } else if (pin[1] == 'I') {
        switch (pin[2]) {
          case '1':
            pinNum = AI1;
            break;
          case '2':
            pinNum = AI2;
            break;
          case '3':
            pinNum = AI3;
            break;
          case '4':
            pinNum = AI4;
            break;
        }
      }
    }

    if (pinNum > 0) {
      if (pin[0] == 'D') {
        subscribeDigital(pinNum, stableTime, host, port, command);
      } else {
        subscribeAnalog(pinNum, stableTime, minVariation, host, port, command);
      }  

      webServer.httpSuccess();
      return;
    }
  }

  webServer.httpFail();
}

void IonoWebClass::subscribeAnalog(int pin, unsigned long stableTime, float minVariation, char *host, int port, char *command) {
  switch (pin) {
    case AV1:
    case AI1:
      _url1.host = host;
      _url1.port = port;
      _url1.command = command;
      break;

    case AV2:
    case AI2:
      _url2.host = host;
      _url2.port = port;
      _url2.command = command;
      break;

    case AV3:
    case AI3:
      _url3.host = host;
      _url3.port = port;
      _url3.command = command;
      break;

    case AV4:
    case AI4:
      _url4.host = host;
      _url4.port = port;
      _url4.command = command;
      break;

    default:
      return;
  }
  Iono.subscribeAnalog(pin, stableTime, minVariation, &callAnalogURL);
}

void IonoWebClass::subscribeDigital(int pin, unsigned long stableTime, char *host, int port, char *command) {
  switch (pin) {
    case DI1:
      _url1.host = host;
      _url1.port = port;
      _url1.command = command;
      break;

    case DI2:
      _url2.host = host;
      _url2.port = port;
      _url2.command = command;
      break;

    case DI3:
      _url3.host = host;
      _url3.port = port;
      _url3.command = command;
      break;

    case DI4:
      _url4.host = host;
      _url4.port = port;
      _url4.command = command;
      break;

    case DI5:
      _url5.host = host;
      _url5.port = port;
      _url5.command = command;
      break;

    case DI6:
      _url6.host = host;
      _url6.port = port;
      _url6.command = command;
      break;

    default:
      return;
  }
  Iono.subscribeDigital(pin, stableTime, &callDigitalURL);
}

void IonoWebClass::callDigitalURL(int pin, float value) {
  char *pinName;
  HttpURL *url;
  switch (pin) {
    case DI1:
      pinName = "DI1";
      url = &_url1;
      break;

    case DI2:
      pinName = "DI2";
      url = &_url2;
      break;

    case DI3:
      pinName = "DI3";
      url = &_url3;
      break;

    case DI4:
      pinName = "DI4";
      url = &_url4;
      break;

    case DI5:
      pinName = "DI5";
      url = &_url5;
      break;

    case DI6:
      pinName = "DI6";
      url = &_url6;
      break;

    default:
      return;
  }

  callURL(url, pinName, value == HIGH ? "1" : "0");
}

void IonoWebClass::callAnalogURL(int pin, float value) {
  char *pinName;
  HttpURL *url;
  switch (pin) {
    case AV1:
      pinName = "AV1";
      url = &_url1;
      break;

    case AI1:
      pinName = "AI1";
      url = &_url1;
      break;

    case AV2:
      pinName = "AV2";
      url = &_url2;
      break;

    case AI2:
      pinName = "AI2";
      url = &_url2;
      break;

    case AV3:
      pinName = "AV3";
      url = &_url3;
      break;

    case AI3:
      pinName = "AI3";
      url = &_url3;
      break;

    case AV4:
      pinName = "AV4";
      url = &_url4;
      break;

    case AI4:
      pinName = "AI4";
      url = &_url4;
      break;

    default:
      return;
  }

  char sVal[6];
  ftoa(sVal, value);
  callURL(url, pinName, sVal);
}

void IonoWebClass::callURL(HttpURL *url, const char *pin, const char *value) {
  String command = String((*url).command);
  command.replace("$pin", pin);
  command.replace("$val", value);

  EthernetClient client;
  for (int i = 0; i < 4; i++) {
    if (client.connect((*url).host, (*url).port)) {
      client.print("GET ");
      client.print(command);
      client.println(" HTTP/1.1");
      client.println("Connection: close");
      client.println();
      break;
    }
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