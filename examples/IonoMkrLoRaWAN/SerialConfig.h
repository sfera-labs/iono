/*
  SerialConfig.h

    Copyright (C) 2018-2022 Sfera Labs S.r.l. - All rights reserved.

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

#define CONSOLE_TIMEOUT 20000
#define _PORT_USB SERIAL_PORT_MONITOR
#define _PORT_RS485 SERIAL_PORT_HARDWARE

class SerialConfig {
  private:
    static Stream *_port;
    static short _spacesCounter;
    static char _inBuffer[64];
    static int _fCntMemAddr;

    static void _close();
    static void _enterConsole();
    // static void _enterConfigWizard();
    static void _exportConfig();
    static bool _importConfig();
    static bool _consumeWhites();
    template <typename T>
    static void _print(T text);
    static void _readEchoLine(int maxLen, bool returnOnMaxLen,
          bool upperCase, int (*charFilter)(int, int, int, int), int p1, int p2);
    static int _betweenFilter(int c, int idx, int min, int max);
    static int _orFilter(int c, int idx, int p1, int p2);
    static int _modesFilter(int c, int idx, int p1, int p2);
    static int _rulesFilter(int c, int idx, int p1, int p2);
    static void _printConfiguration(char* devAddr, char* nwkSKey, char* appSKey,
        _lora_band band, uint8_t dataRate, char *modes, char *rules);
    static void _confirmConfiguration(char* devAddr, char* nwkSKey, char* appSKey,
        _lora_band band, uint8_t dataRate, char *modes, char *rules);
    static bool _readEepromConfig();
    static bool _writeEepromConfig(char* devAddr, char* nwkSKey, char* appSKey,
        _lora_band band, uint8_t dataRate, char *modes, char *rules);

  public:
    static bool isConfigured;
    static bool isAvailable;

    static uint32_t fCntUp;
    static uint32_t fCntDown;

    static char devAddr[9];
    static char nwkSKey[33];
    static char appSKey[33];
    static _lora_band band;
    static uint8_t dataRate;
    static char modes[7];
    static char rules[5];

    static void setup();
    static void process();
    static void writeFCntUp(uint32_t);
    static void writeFCntDown(uint32_t);
};

bool SerialConfig::isConfigured = false;
bool SerialConfig::isAvailable = true;

uint32_t SerialConfig::fCntUp = 0;
uint32_t SerialConfig::fCntDown = 0;

Stream *SerialConfig::_port = NULL;
short SerialConfig::_spacesCounter = 0;
char SerialConfig::_inBuffer[64];
int SerialConfig::_fCntMemAddr;

char SerialConfig::devAddr[9];
char SerialConfig::nwkSKey[33];
char SerialConfig::appSKey[33];
_lora_band SerialConfig::band;
uint8_t SerialConfig::dataRate;
char SerialConfig::modes[7];
char SerialConfig::rules[5];

void SerialConfig::setup() {
  _PORT_USB.begin(9600);
  _PORT_RS485.begin(9600);

  isConfigured = _readEepromConfig();

  if (!isConfigured) {
    for (int i = 0; i < 32; i++) {
      if (i < 8) {
        devAddr[i] = '0';
      }
      nwkSKey[i] = '0';
      appSKey[i] = '0';
    }
    devAddr[8] = '\0';
    nwkSKey[32] = '\0';
    appSKey[32] = '\0';
    band = EU868;
    dataRate = 5;
    strncpy(modes, "DDDDDD", 6);
    modes[6] = '\0';
    strncpy(rules, "----", 4);
    rules[4] = '\0';
  }
}

void SerialConfig::process() {
  if (_port == NULL) {
    if (_PORT_USB.available()) {
      _port = &_PORT_USB;
    } else if (_PORT_RS485.available()) {
      _port = &_PORT_RS485;
    }
  }

  while (_port != NULL && _port->available()) {
    int b = _port->read();
    if (b == ' ') {
      if (_spacesCounter >= 4) {
        _enterConsole();
      } else {
        _spacesCounter++;
      }
    } else if (isConfigured) {
      _close();
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
    _print("=== Sfera Labs - Iono MKR LoRaWAN configuration - v1.0.0 ===\r\n"
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

bool SerialConfig::_importConfig() {
  char devAddrNew[9];
  char nwkSKeyNew[33];
  char appSKeyNew[33];
  _lora_band bandNew = EU868;
  uint8_t dataRateNew = 5;
  char modesNew[7];
  char rulesNew[5];
  
  String l;
  int n;
  
  devAddrNew[0] = '\0';
  nwkSKeyNew[0] = '\0';
  appSKeyNew[0] = '\0';
  modesNew[0] = '\0';
  rulesNew[0] = '\0';

  _print("\r\nPaste the configuration:\r\n");
  if (!_consumeWhites()) {
    return false;
  }
  _port->setTimeout(300);
  while (true) {
    l = _port->readStringUntil(':');
    
    if (l.endsWith("devAddr")) {
      if (!_consumeWhites()) {
        return false;
      }
      n = _port->readBytes(devAddrNew, 8);
      if (n != 8) {
        return false;
      }
      devAddrNew[8] = '\0';
      
    } else if (l.endsWith("nwkSKey")) {
      if (!_consumeWhites()) {
        return false;
      }
      n = _port->readBytes(nwkSKeyNew, 32);
      if (n != 32) {
        return false;
      }
      nwkSKeyNew[32] = '\0';
      
    } else if (l.endsWith("appSKey")) {
      if (!_consumeWhites()) {
        return false;
      }
      n = _port->readBytes(appSKeyNew, 32);
      if (n != 32) {
        return false;
      }
      appSKeyNew[32] = '\0';
      
    } else if (l.endsWith("rate")) {
      dataRateNew = _port->parseInt();

    } else if (l.endsWith("band")) {
      if (!_consumeWhites()) {
        return false;
      }
      n = _port->readBytes(_inBuffer, 2);
      if (n != 2) {
        return false;
      }
      if (_inBuffer[0] == 'A' && _inBuffer[1] == 'S') {
        bandNew = AS923;
      } else if (_inBuffer[0] == 'A' && _inBuffer[1] == 'U') {
        bandNew = AU915;
      } else if (_inBuffer[0] == 'E' && _inBuffer[1] == 'U') {
        bandNew = EU868;
      } else if (_inBuffer[0] == 'K' && _inBuffer[1] == 'R') {
        bandNew = KR920;
      } else if (_inBuffer[0] == 'I' && _inBuffer[1] == 'N') {
        bandNew = IN865;
      } else if (_inBuffer[0] == 'U' && _inBuffer[1] == 'S') {
        bandNew = US915;
      } else {
        return false;
      }
      
    } else if (l.endsWith("modes")) {
      if (!_consumeWhites()) {
        return false;
      }
      n = _port->readBytes(modesNew, 6);
      if (n != 6) {
        return false;
      }
      modesNew[6] = '\0';
      
    } else if (l.endsWith("rules")) {
      if (!_consumeWhites()) {
        return false;
      }
      n = _port->readBytes(rulesNew, 4);
      if (n != 4) {
        return false;
      }
      rulesNew[4] = '\0';
      
    } else {
      break;
    }
  }

  if (devAddrNew[0] == '\0' || nwkSKeyNew[0] == '\0' || appSKeyNew[0] == '\0' || 
      modesNew[0] == '\0' || rulesNew[0] == '\0') {
    return false;
  }

  _confirmConfiguration(devAddrNew, nwkSKeyNew, appSKeyNew,
    bandNew, dataRateNew, modesNew, rulesNew);
}

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
  _printConfiguration(devAddr, nwkSKey, appSKey,
    band, dataRate, modes, rules);
  _print("\r\n");
}

template <typename T>
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

int SerialConfig::_modesFilter(int c, int idx, int p1, int p2) {
  if (c == 'D' || c == '-' || (idx < 4 && (c == 'V' || c == 'I'))) {
    return c;
  }
  return -1;
}

int SerialConfig::_rulesFilter(int c, int idx, int p1, int p2) {
  if (c == 'F' || c == 'I' || c == 'H' || c == 'L' || c == 'T' || c == '-') {
    return c;
  }
  return -1;
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

bool SerialConfig::_writeEepromConfig(char* devAddr, char* nwkSKey, char* appSKey,
    _lora_band band, uint8_t dataRate, char *modes, char *rules) {
  byte checksum = 7;
  int a = 2;
  for (int i = 0; i < 8; i++) {
    EEPROM.write(a++, devAddr[i]);
    checksum ^= devAddr[i];
  }
  for (int i = 0; i < 32; i++) {
    EEPROM.write(a++, nwkSKey[i]);
    checksum ^= nwkSKey[i];
  }
  for (int i = 0; i < 32; i++) {
    EEPROM.write(a++, appSKey[i]);
    checksum ^= appSKey[i];
  }
  EEPROM.write(a++, band);
  checksum ^= band;
  EEPROM.write(a++, dataRate);
  checksum ^= dataRate;
  for (int i = 0; i < 6; i++) {
    EEPROM.write(a++, modes[i]);
    checksum ^= modes[i];
  }
  for (int i = 0; i < 4; i++) {
    EEPROM.write(a++, rules[i]);
    checksum ^= rules[i];
  }

  EEPROM.write(0, a - 2);
  checksum ^= a - 2;
  EEPROM.write(1, checksum);

  // fCntUp & fCntDown reset
  for (int i = 0; i < 8; i++) {
    EEPROM.write(a + i, 0);
  }

  EEPROM.commit();

  return true;
}

void SerialConfig::writeFCntUp(uint32_t fCntUp) {
  EEPROM.write(_fCntMemAddr, (byte) (fCntUp >> 24));
  EEPROM.write(_fCntMemAddr + 1, (byte) (fCntUp >> 16));
  EEPROM.write(_fCntMemAddr + 2, (byte) (fCntUp >> 8));
  EEPROM.write(_fCntMemAddr + 3, (byte) fCntUp);
  EEPROM.commit();
}

void SerialConfig::writeFCntDown(uint32_t fCntDown) {
  EEPROM.write(_fCntMemAddr + 4, (byte) (fCntDown >> 24));
  EEPROM.write(_fCntMemAddr + 5, (byte) (fCntDown >> 16));
  EEPROM.write(_fCntMemAddr + 6, (byte) (fCntDown >> 8));
  EEPROM.write(_fCntMemAddr + 7, (byte) fCntDown);
  EEPROM.commit();
}

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
  for (int i = 0; i < 8; i++) {
    devAddr[i] = mem[a++];
  }
  for (int i = 0; i < 32; i++) {
    nwkSKey[i] = mem[a++];
  }
  for (int i = 0; i < 32; i++) {
    appSKey[i] = mem[a++];
  }
  band = (_lora_band) mem[a++];
  dataRate = mem[a++];
  for (int i = 0; i < 6; i++) {
    modes[i] = mem[a++];
  }
  for (int i = 0; i < 4; i++) {
    rules[i] = mem[a++];
  }

  _fCntMemAddr = a + 2;
  fCntUp = ((EEPROM.read(_fCntMemAddr) & 0xfful) << 24) + ((EEPROM.read(_fCntMemAddr + 1) & 0xfful) << 16) + ((EEPROM.read(_fCntMemAddr + 2) & 0xfful) << 8) + (EEPROM.read(_fCntMemAddr + 3) & 0xfful);
  fCntDown = ((EEPROM.read(_fCntMemAddr + 4) & 0xfful) << 24) + ((EEPROM.read(_fCntMemAddr + 5) & 0xfful) << 16) + ((EEPROM.read(_fCntMemAddr + 6) & 0xfful) << 8) + (EEPROM.read(_fCntMemAddr + 7) & 0xfful);

  return true;
}

void SerialConfig::_confirmConfiguration(char* devAddr, char* nwkSKey, char* appSKey,
    _lora_band band, uint8_t dataRate, char *modes, char *rules) {

  _print("\r\nNew configuration:\r\n");

  _printConfiguration(devAddr, nwkSKey, appSKey,
    band, dataRate, modes, rules);

  _print("\r\nConfirm? (Y/N):\r\n\r\n");
  do {
    _print("> ");
    _readEchoLine(1, false, true, &_orFilter, 'Y', 'N');
    if (_inBuffer[0] == 'Y') {
      _print("\r\nSaving...");
      _writeEepromConfig(devAddr, nwkSKey, appSKey, band, dataRate, modes, rules);
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

void SerialConfig::_printConfiguration(char* devAddr, char* nwkSKey, char* appSKey,
    _lora_band band, uint8_t dataRate, char *modes, char *rules) {
  _print("\r\ndevAddr: ");
  _print(devAddr);
  _print("\r\nnwkSKey: ");
  _print(nwkSKey);
  _print("\r\nappSKey: ");
  _print(appSKey);
  _print("\r\nband: ");
  switch(band) {
    case AS923: _print("AS"); break;
    case AU915: _print("AU"); break;
    case EU868: _print("EU"); break;
    case KR920: _print("KR"); break;
    case IN865: _print("IN"); break;
    case US915: _print("US"); break;
  }
  _print("\r\ndata rate: ");
  _print(dataRate);
  _print("\r\ninput modes: ");
  _print(modes);
  _print("\r\nI/O rules: ");
  _print(rules);
  
  _print("\r\n");
}

extern SerialConfig SerialConfig;

#endif
