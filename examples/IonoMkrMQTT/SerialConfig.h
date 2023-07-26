/*
  SerialConfig.h

    Copyright (C) 2020-2023 Sfera Labs S.r.l. - All rights reserved.

    For information, see:
    https://www.sferalabs.cc/

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#ifndef SerialConfig_h
#define SerialConfig_h

#include <FlashAsEEPROM.h>
#include <FlashStorage.h>
#include "Watchdog.h"

#define CONSOLE_TIMEOUT 10000
#define _PORT_USB SERIAL_PORT_MONITOR
#define _PORT_RS485 SERIAL_PORT_HARDWARE

class SerialConfig {
  private:
    static Stream *_port;
    static short _spacesCounter;
    static char _inBuffer[64];

    static void _close();
    static void _enterConsole();
    static void _exportConfig();
    static bool _importConfig();
    static bool _consumeWhites();
    static String _readNextField();
    template <typename T>
    static void _print(T text);
    static void _readEchoLine(int maxLen, bool returnOnMaxLen,
          bool upperCase, int (*charFilter)(int, int, int, int), int p1, int p2);
    static int _betweenFilter(int c, int idx, int min, int max);
    static int _orFilter(int c, int idx, int p1, int p2);
    static bool _modesFilter(String s);
    static bool _rulesFilter(String s);
    static void _printConfiguration(char* ssid, char* netpass, char* brokerAddr, char* numPort, char* modes, char* rules, char* keepAlive, char qos, char retain, char watchdog, char* willTopic, char* willPayload, char* clientId,
        char *username, char *password, char* rootTopic);
    static void _confirmConfiguration(char* ssid, char* netpass, char* brokerAddr, char* numPort, char* modes, char* rules, char* keepAlive, char qos, char retain, char watchdog, char* willTopic, char* willPayload, char* clientId,
        char *username, char *password, char* rootTopic);
    static bool _readEepromConfig();
    static bool _writeEepromConfig(char* ssid, char* netpass, char* brokerAddr, char* numPort, char* modes, char* rules, char* keepAlive, char qos, char retain, char watchdog, char* willTopic, char* willPayload, char* clientId,
        char *username, char *password, char* rootTopic);

  public:
    static bool isConfigured;
    static bool isAvailable;

    static char ssid[101];
    static char netpass[101];
    static char brokerAddr[16];
    static char numPort[9];
    static char modes[7];
    static char rules[5];
    static char keepAlive[9];
    static char qos;
    static char retain;
    static char watchdog;
    static char willTopic[101];
    static char willPayload[101];
    static char clientId[101];
    static char username[101];
    static char password[101];
    static char rootTopic[101];

    static void setup();
    static void process();
};

bool SerialConfig::isConfigured = false;
bool SerialConfig::isAvailable = true;

Stream *SerialConfig::_port = NULL;
short SerialConfig::_spacesCounter = 0;
char SerialConfig::_inBuffer[64];

char SerialConfig::ssid[101];
char SerialConfig::netpass[101];
char SerialConfig::brokerAddr[16];
char SerialConfig::numPort[9];
char SerialConfig::modes[7];
char SerialConfig::rules[5];
char SerialConfig::keepAlive[9];
char SerialConfig::qos;
char SerialConfig::retain;
char SerialConfig::watchdog;
char SerialConfig::willTopic[101];
char SerialConfig::willPayload[101];
char SerialConfig::clientId[101];
char SerialConfig::username[101];
char SerialConfig::password[101];
char SerialConfig::rootTopic[101];

void SerialConfig::setup() {
  _PORT_USB.begin(9600);
  _PORT_RS485.begin(9600);

  isConfigured = _readEepromConfig();

  if (!isConfigured) {
    String l = "networkname";
    l.toCharArray(ssid, sizeof(ssid));
    ssid[l.length()] = '\0';
    l = "networkpassword";
    l.toCharArray(netpass, sizeof(netpass));
    netpass[l.length()] = '\0';
    l = "192.168.1.128";
    l.toCharArray(brokerAddr, sizeof(brokerAddr));
    brokerAddr[l.length()] = '\0';
    l = "1883";
    l.toCharArray(numPort, sizeof(numPort));
    numPort[l.length()] = '\0';
    l = "DDDDDD";
    l.toCharArray(modes, sizeof(modes));
    modes[l.length()] = '\0';
    l = "F--I";
    l.toCharArray(rules, sizeof(rules));
    rules[l.length()] = '\0';
    l = "120";
    l.toCharArray(keepAlive, sizeof(keepAlive));
    keepAlive[l.length()] = '\0';
    qos = '1';
    retain = 'F';
    watchdog = 'F';
    l = "will";
    l.toCharArray(willTopic, sizeof(willTopic));
    willTopic[l.length()] = '\0';
    l = "goodbye";
    l.toCharArray(willPayload, sizeof(willPayload));
    willPayload[l.length()] = '\0';
    l = "sensor1";
    l.toCharArray(clientId, sizeof(clientId));
    clientId[l.length()] = '\0';
    l = "user";
    l.toCharArray(username, sizeof(username));
    username[l.length()] = '\0';
    l = "pass";
    l.toCharArray(password, sizeof(password));
    password[l.length()] = '\0';
    l = "/iono-mkr/";
    l.toCharArray(rootTopic, sizeof(rootTopic));
    rootTopic[l.length()] = '\0';
  }
}

void SerialConfig::process() {
  // set port usb or rs485
  if (_port == NULL) {
    if (_PORT_USB.available()) {
      _port = &_PORT_USB;
    } else if (_PORT_RS485.available()) {
      _port = &_PORT_RS485;
    }
  }

  // enter serial configuration mode
  while (_port != NULL && _port->available()) {
    int b = _port->read();
    if (b == ' ') {
      if (_spacesCounter >= 4) {
        _enterConsole();
      } else {
        _spacesCounter++;
      }
    } else {
      _port = NULL;
    }
  }

  if (isConfigured && millis() > CONSOLE_TIMEOUT) {
    _close();
  }
}

void SerialConfig::_close() {
  isAvailable = false;
  _PORT_USB.end();
  _PORT_RS485.end();
}

void SerialConfig::_enterConsole() {
  Watchdog.disable();
  delay(100);
  while(_port->read() >= 0) {
    delay(5);
  }
  while (true) {
    _print("=== Sfera Labs - Iono MKR MQTT configuration - v1.2 ===\r\n"
           /* "\r\n    1. Configuration wizard" */
           "\r\n    1. Import configuration"
           "\r\n    2. Export configuration"
           "\r\n\r\n> "
         );
    _readEchoLine(1, false, false, &_betweenFilter, '1', '2');

    switch (_inBuffer[0]) {
      /*
      case '1':
        _enterConfigWizard();
        break;
      */
      case '1':
        if (!_importConfig()) {
          while(_port->read() >= 0) {
            delay(5);
          }
          _print("\r\nError\r\n\r\n");
        }
        break;
      case '2':
        _exportConfig();
        break;
      default:
        break;
    }
  }
}

// read serial configuration
bool SerialConfig::_importConfig() {
  char ssidNew[101];
  char netpassNew[101];
  char brokerAddrNew[16];
  char numPortNew[9];
  char modesNew[7];
  char rulesNew[5];
  char keepAliveNew[9] = {'6', '0', '\0'};
  char qosNew;
  char retainNew;
  char watchdogNew;
  char willTopicNew[101];
  char willPayloadNew[101];
  char clientIdNew[101];
  char usernameNew[101];
  char passwordNew[101];
  char rootTopicNew[101];

  String l, f;
  int n;
  char mode, ret;

  ssidNew[0] = '\0';
  netpassNew[0] = '\0';
  brokerAddrNew[0] = '\0';
  numPortNew[0] = '\0';
  modesNew[0] = '\0';
  rulesNew[0] = '\0';
  qosNew = '0';
  retainNew = 'F';
  watchdogNew = 'F';
  willTopicNew[0] = '\0';
  willPayloadNew[0] = '\0';
  clientIdNew[0] = '\0';
  usernameNew[0] = '\0';
  passwordNew[0] = '\0';
  rootTopicNew[0] = '\0';

  _print("\r\nPaste the configuration:\r\n");

  if (!_consumeWhites()) {
    return false;
  }

  _port->setTimeout(300);
  while (true) {
    l = _port->readStringUntil(':');

    if (l.endsWith("netssid")) {
      if (!_consumeWhites())
        return false;
      f = _readNextField();
      if (f.length() > 100)
        return false;
      for (int i = 0; i < f.length(); i++)
        ssidNew[i] = f.charAt(i);
      ssidNew[f.length()] = '\0';

    } else if (l.endsWith("netpass")) {
      if (!_consumeWhites())
        return false;
      f = _readNextField();
      if (f.length() > 100)
        return false;
      for (int i = 0; i < f.length(); i++)
        netpassNew[i] = f.charAt(i);
      netpassNew[f.length()] = '\0';

    } else if (l.endsWith("brokeraddr")) {
      if (!_consumeWhites())
        return false;
      f = _readNextField();
      if (f.length() > 15)
        return false;
      for (int i = 0; i < f.length(); i++)
        brokerAddrNew[i] = f.charAt(i);
      brokerAddrNew[f.length()] = '\0';

    } else if (l.endsWith("brokerport")) {
      if (!_consumeWhites())
        return false;
      f = _readNextField();
      if (f.length() > 8)
        return false;
      for (int i = 0; i < f.length(); i++)
        numPortNew[i] = f.charAt(i);
      numPortNew[f.length()] = '\0';

    } else if (l.endsWith("modes")) {
      if (!_consumeWhites())
        return false;
      f = _readNextField();
      if (f.length() != 6 || !_modesFilter(f))
        return false;
      for (int i = 0; i < f.length(); i++)
        modesNew[i] = f.charAt(i);
      modesNew[f.length()] = '\0';

    } else if (l.endsWith("rules")) {
      if (!_consumeWhites())
        return false;
      f = _readNextField();
      if (f.length() != 4 || !_rulesFilter(f))
        return false;
      for (int i = 0; i < f.length(); i++)
        rulesNew[i] = f.charAt(i);
      rulesNew[f.length()] = '\0';

    } else if (l.endsWith("keepalive")) {
      if (!_consumeWhites())
        return false;
      f = _readNextField();
      if (f.length() > 8)
        return false;
      for (int i = 0; i < f.length(); i++) {
        if (!isDigit(f.charAt(i)))
          return false;
        keepAliveNew[i] = f.charAt(i);
      }
      keepAliveNew[f.length()] = '\0';

    } else if (l.endsWith("qos")) {
      if (!_consumeWhites())
        return false;
      f = _readNextField();
      ret = f.charAt(0);
      if (ret != '0' && ret != '1' && ret != '2')
        return false;
      qosNew = ret;

    } else if (l.endsWith("retain")) {
      if (!_consumeWhites())
        return false;
      f = _readNextField();
      ret = f.charAt(0);
      if (ret != 'T' && ret != 'F')
        return false;
      retainNew = ret;

    } else if (l.endsWith("watchdog")) {
      if (!_consumeWhites())
        return false;
      f = _readNextField();
      ret = f.charAt(0);
      if (ret != 'T' && ret != 'F')
        return false;
      watchdogNew = ret;

    } else if (l.endsWith("willtopic")) {
      if (!_consumeWhites())
        return false;
      f = _readNextField();
      if (f.length() > 100)
        return false;
      for (int i = 0; i < f.length(); i++)
        willTopicNew[i] = f.charAt(i);
      willTopicNew[f.length()] = '\0';

    } else if (l.endsWith("willpayload")) {
      if (!_consumeWhites())
        return false;
      f = _readNextField();
      if (f.length() > 100)
        return false;
      for (int i = 0; i < f.length(); i++)
        willPayloadNew[i] = f.charAt(i);
      willPayloadNew[f.length()] = '\0';

    } else if (l.endsWith("clientId")) {
      if (!_consumeWhites())
        return false;
      f = _readNextField();
      if (f.length() > 100)
        return false;
      for (int i = 0; i < f.length(); i++)
        clientIdNew[i] = f.charAt(i);
      clientIdNew[f.length()] = '\0';

    } else if (l.endsWith("username")) {
      if (!_consumeWhites())
        return false;
      f = _readNextField();
      if (f.length() > 100)
        return false;
      for (int i = 0; i < f.length(); i++)
        usernameNew[i] = f.charAt(i);
      usernameNew[f.length()] = '\0';

    } else if (l.endsWith("password")) {
      if (!_consumeWhites())
        return false;
      f = _readNextField();
      if (f.length() > 100)
        return false;
      for (int i = 0; i < f.length(); i++)
        passwordNew[i] = f.charAt(i);
      passwordNew[f.length()] = '\0';

    } else if (l.endsWith("roottopic")) {
      if (!_consumeWhites())
        return false;
      f = _readNextField();
      if (f.length() > 100)
        return false;
      for (int i = 0; i < f.length(); i++)
        rootTopicNew[i] = f.charAt(i);
      rootTopicNew[f.length()] = '\0';

    } else {
      break;
    }
  }

  if (ssidNew[0] == '\0' || netpassNew[0] == '\0' || brokerAddrNew[0] == '\0' || numPortNew[0] == '\0' || modesNew[0] == '\0' || rulesNew[0] == '\0' || clientIdNew[0] == '\0' ||
      usernameNew[0] == '\0' || passwordNew[0] == '\0') {
    return false;
  }

  _confirmConfiguration(ssidNew, netpassNew, brokerAddrNew, numPortNew, modesNew, rulesNew, keepAliveNew, qosNew, retainNew, watchdogNew, willTopicNew, willPayloadNew,
    clientIdNew, usernameNew, passwordNew, rootTopicNew);
}

// consume whitespaces
bool SerialConfig::_consumeWhites() {
  int c;
  while (true) {
    c = _port->peek();
    if (c >= 0) {
      if (c == '\b' || c == 127 || c == 27) {
        return false;
      }
      if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
        break;
      }
      _port->read();
    }
  }
  return true;
}

void SerialConfig::_exportConfig() {
  if (!isConfigured) {
    _print("\r\n*** Not configured ***\r\n\r\nExample:");
  }
  _print("\r\n");
  _printConfiguration(ssid, netpass, brokerAddr, numPort, modes, rules, keepAlive, qos, retain, watchdog, willTopic, willPayload,
    clientId, username, password, rootTopic);
  _print("\r\n");
}

template <typename T>
// write when transmission pin in enabled
void SerialConfig::_print(T text) {
  digitalWrite(PIN_TXEN, HIGH);
  _port->print(text);
  _port->flush();
  delay(5);
  digitalWrite(PIN_TXEN, LOW);
}

int SerialConfig::_orFilter(int c, int idx, int p1, int p2) {
  if (c == p1 || c == p2) {
    return c;
  }
  return -1;
}

int SerialConfig::_betweenFilter(int c, int idx, int min, int max) {
  if (c >= min && c <= max) {
    return c;
  }
  return -1;
}

bool SerialConfig::_modesFilter(String s) {
  char mode;
  for (int i = 0; i < 4; i++) {
    mode = s.charAt(i);
    if (mode != 'D' && mode != 'V' && mode != 'I' && mode != '-')
      return false;
  }
  for (int i = 4; i < 6; i++) {
    mode = s.charAt(i);
    if (mode != 'D' && mode != '-')
      return false;
  }
  return true;
}

bool SerialConfig::_rulesFilter(String s) {
  char rule;
  for (int i = 0; i < 4; i++) {
    rule = s.charAt(i);
    if (rule != 'F' && rule != 'I' && rule != 'H' && rule != 'L' && rule != 'T' && rule != '-')
      return false;
  }
  return true;
}

void SerialConfig::_readEchoLine(int maxLen, bool returnOnMaxLen,
      bool upperCase, int (*charFilter)(int, int, int, int), int p1, int p2) {
  int c, i = 0, p = 0;
  bool eol = false;
  while (true) {
    while (_port->available() && !eol) {
      c = _port->read();
      switch (c) {
        case '\r':
        case '\n':
          eol = true;
          break;
        case '\b':
        case 127:
          if (i > 0) {
            i--;
          }
          break;
        default:
          if (i < maxLen) {
            if (upperCase && c >= 'a') {
              c -= 32;
            }
            c = charFilter(c, i, p1, p2);
            if (c >= 0) {
              _inBuffer[i++] = c;
            }
          }
          if (returnOnMaxLen && i >= maxLen) {
            eol = true;
          }
          break;
      }
      delay(5);
    }

    for (; p < i; p++) {
      _print(_inBuffer[p]);
    }
    for (; p > i; p--) {
      _print("\b \b");
    }
    if (eol) {
      _inBuffer[i] = '\0';
      _print("\r\n");
      return;
    }
  }
}

// write configuration on board memory
bool SerialConfig::_writeEepromConfig(char* ssid, char* netpass, char* brokerAddr, char* numPort, char* modes, char* rules, char* keepAlive,
    char qos, char retain, char watchdog, char* willTopic, char* willPayload, char* clientId, char *username, char *password, char* rootTopic) {
  byte checksum = 7;
  int a = 2;

  for (int i = 0; i < 100; i++) {
    EEPROM.write(a++, ssid[i]);
    checksum ^= ssid[i];
    if (ssid[i] == '\0') {
      break;
    }
  }

  for (int i = 0; i < 100; i++) {
    EEPROM.write(a++, netpass[i]);
    checksum ^= netpass[i];
    if (netpass[i] == '\0') {
      break;
    }
  }

  for (int i = 0; i < 15; i++) {
    EEPROM.write(a++, brokerAddr[i]);
    checksum ^= brokerAddr[i];
    if (brokerAddr[i] == '\0') {
      break;
    }
  }

  for (int i = 0; i < 8; i++) {
    EEPROM.write(a++, numPort[i]);
    checksum ^= numPort[i];
  }

  for (int i = 0; i < 6; i++) {
    EEPROM.write(a++, modes[i]);
    checksum ^= modes[i];
  }

  for (int i = 0; i < 4; i++) {
    EEPROM.write(a++, rules[i]);
    checksum ^= rules[i];
  }

  for (int i = 0; i < 8; i++) {
    EEPROM.write(a++, keepAlive[i]);
    checksum ^= keepAlive[i];
  }

  EEPROM.write(a++, qos);
  checksum ^= qos;

  EEPROM.write(a++, retain);
  checksum ^= retain;

  EEPROM.write(a++, watchdog);
  checksum ^= watchdog;

  for (int i = 0; i < 100; i++) {
    EEPROM.write(a++, willTopic[i]);
    checksum ^= willTopic[i];
    if (willTopic[i] == '\0') {
      break;
    }
  }

  for (int i = 0; i < 100; i++) {
    EEPROM.write(a++, willPayload[i]);
    checksum ^= willPayload[i];
    if (willPayload[i] == '\0') {
      break;
    }
  }

  for (int i = 0; i < 100; i++) {
    EEPROM.write(a++, clientId[i]);
    checksum ^= clientId[i];
    if (clientId[i] == '\0') {
      break;
    }
  }

  for (int i = 0; i < 100; i++) {
    EEPROM.write(a++, username[i]);
    checksum ^= username[i];
    if (username[i] == '\0') {
      break;
    }
  }

  for (int i = 0; i < 100; i++) {
    EEPROM.write(a++, password[i]);
    checksum ^= password[i];
    if (password[i] == '\0') {
      break;
    }
  }

  for (int i = 0; i < 100; i++) {
    EEPROM.write(a++, rootTopic[i]);
    checksum ^= rootTopic[i];
    if (rootTopic[i] == '\0') {
      break;
    }
  }

  EEPROM.write(0, a - 2);
  checksum ^= a - 2;
  EEPROM.write(1, checksum);

  EEPROM.commit();

  return true;
}

// read configuration from board memory
bool SerialConfig::_readEepromConfig() {
  if (!EEPROM.isValid()) {
    return false;
  }

  byte checksum = 7;
  int len = EEPROM.read(0) & 0xff;
  byte mem[len];
  for (int i = 0; i < len; i++) {
    mem[i] = EEPROM.read(i + 2);
    checksum ^= mem[i];
  }
  checksum ^= len;
  if ((EEPROM.read(1) != checksum)) {
    return false;
  }

  int a = 0;

  for (int i = 0; i < 100; i++) {
    ssid[i] = mem[a++];
    if (ssid[i] == '\0') {
      break;
    }
  }

  for (int i = 0; i < 100; i++) {
    netpass[i] = mem[a++];
    if (netpass[i] == '\0') {
      break;
    }
  }

  for (int i = 0; i < 15; i++) {
    brokerAddr[i] = mem[a++];
    if (brokerAddr[i] == '\0') {
      break;
    }
  }

  for (int i = 0; i < 8; i++) {
    numPort[i] = mem[a++];
  }

  for (int i = 0; i < 6; i++) {
    modes[i] = mem[a++];
  }

  for (int i = 0; i < 4; i++) {
    rules[i] = mem[a++];
  }

  for (int i = 0; i < 8; i++) {
    keepAlive[i] = mem[a++];
  }

  qos = mem[a++];

  retain = mem[a++];

  watchdog = mem[a++];

  for (int i = 0; i < 100; i++) {
    willTopic[i] = mem[a++];
    if (willTopic[i] == '\0') {
      break;
    }
  }

  for (int i = 0; i < 100; i++) {
    willPayload[i] = mem[a++];
    if (willPayload[i] == '\0') {
      break;
    }
  }

  for (int i = 0; i < 100; i++) {
    clientId[i] = mem[a++];
    if (clientId[i] == '\0') {
      break;
    }
  }

  for (int i = 0; i < 100; i++) {
    username[i] = mem[a++];
    if (username[i] == '\0') {
      break;
    }
  }

  for (int i = 0; i < 100; i++) {
    password[i] = mem[a++];
    if (password[i] == '\0')
      break;
  }

  for (int i = 0; i < 100; i++) {
    rootTopic[i] = mem[a++];
    if (rootTopic[i] == '\0')
      break;
  }

  return true;
}

// check configuration arguments and save if correct
void SerialConfig::_confirmConfiguration(char* ssid, char* netpass, char* brokerAddr, char* numPort, char* modes, char* rules, char* keepAlive,
    char qos, char retain, char watchdog, char* willTopic, char* willPayload, char* clientId, char *username, char *password, char* rootTopic) {

  _print("\r\nNew configuration:\r\n");

  _printConfiguration(ssid, netpass, brokerAddr, numPort, modes, rules, keepAlive, qos, retain, watchdog, willTopic, willPayload, clientId,
    username, password, rootTopic);

  _print("\r\nConfirm? (Y/N):\r\n\r\n");
  do {
    _print("> ");
    _readEchoLine(1, false, true, &_orFilter, 'Y', 'N');
    if (_inBuffer[0] == 'Y') {
      _print("\r\nSaving...");
      _writeEepromConfig(ssid, netpass, brokerAddr, numPort, modes, rules, keepAlive, qos, retain, watchdog, willTopic, willPayload,
          clientId, username, password, rootTopic);
      if (_readEepromConfig()) {
        _print("\r\nSaved!\r\nResetting... bye!\r\n\r\n");
        delay(1000);
        NVIC_SystemReset();
      } else {
        _print("\r\nError\r\n\r\n");
      }
      break;
    } else if (_inBuffer[0] == 'N') {
      break;
    }
  } while (true);
}

void SerialConfig::_printConfiguration(char* ssid, char* netpass, char* brokerAddr, char* numPort, char* modes, char* rules, char* keepAlive,
    char qos, char retain, char watchdog, char* willTopic, char* willPayload, char* clientId, char *username, char *password, char* rootTopic) {
  _print("\r\nnetssid: ");
  _print(ssid);
  _print("\r\nnetpass: ");
  _print(netpass);
  _print("\r\nbrokeraddr: ");
  _print(brokerAddr);
  _print("\r\nbrokerport: ");
  _print(numPort);
  _print("\r\nmodes: ");
  _print(modes);
  _print("\r\nrules: ");
  _print(rules);
  _print("\r\nkeepalive: ");
  _print(keepAlive);
  _print("\r\nqos: ");
  _print(qos);
  _print("\r\nretain: ");
  _print(retain);
  _print("\r\nwilltopic: ");
  _print(willTopic);
  _print("\r\nwillpayload: ");
  _print(willPayload);
  _print("\r\nclientId: ");
  _print(clientId);
  _print("\r\nusername: ");
  _print(username);
  _print("\r\npassword: ");
  _print(password);
  _print("\r\nroottopic: ");
  _print(rootTopic);
  _print("\r\nwatchdog: ");
  _print(watchdog);

  _print("\r\n");
}

String SerialConfig::_readNextField() {
  String field = "";
  char c;
  while (_port->available()) {
    c = _port->read();
    if (c >= 0) {
      if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        break;
      else
        field += (char) c;
    }
    delay(5);
  }
  return field;
}

extern SerialConfig SerialConfig;

#endif
