/*
  ModbusRtuApp.cpp - A configurable Modbus RTU Slave for Iono Arduino and Iono MKR

    Copyright (C) 2016-2018 Sfera Labs S.r.l. - All rights reserved.

    For information, see:
    http://www.sferalabs.cc/iono-arduino

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#include <Iono.h>
#include <IonoModbusRtuSlave.h>

#ifdef ARDUINO_ARCH_SAMD
#include <FlashAsEEPROM.h>
#include <FlashStorage.h>
#else
#include <EEPROM.h>
#endif

#define DELAY  25                           // the debounce delay in milliseconds
#define BOOT_CONSOLE_TIMEOUT_MILLIS 15000   // if 5 consecutive spaces are received within this time interval after boot, enter console mode

const PROGMEM char CONSOLE_MENU_HEADER[]  = {"Sfera Labs - Iono Arduino/MKR - Modbus RTU Slave configuration menu"};
const PROGMEM char CONSOLE_MENU_CURRENT_CONFIG[]  = {"Print current configuration"};
const PROGMEM char CONSOLE_MENU_SPEED[]  = {"Speed (baud)"};
const PROGMEM char CONSOLE_MENU_PARITY[]  = {"Parity"};
const PROGMEM char CONSOLE_MENU_MIRROR[]  = {"Input/Output rules"};
const PROGMEM char CONSOLE_MENU_ADDRESS[]  = {"Modbus device address"};
const PROGMEM char CONSOLE_MENU_SAVE[]  = {"Save configuration and restart"};
const PROGMEM char CONSOLE_MENU_TYPE[]  = {"Type a menu number (0, 1, 2, 3, 4, 5): "};
const PROGMEM char CONSOLE_TYPE_SPEED[]  = {"Type serial port speed (1: 1200, 2: 2400, 3: 4800, 4: 9600; 5: 19200; 6: 38400, 7: 57600, 8: 115200): "};
const PROGMEM char CONSOLE_TYPE_PARITY[]  = {"Type serial port parity (1: Even, 2: Odd, 3: None): "};
const PROGMEM char CONSOLE_TYPE_ADDRESS[]  = {"Type Modbus device address (1-247): "};
const PROGMEM char CONSOLE_TYPE_MIRROR[]  = {"Type Input/Output rules (xxxxxx, F: follow, I: invert, H: flip on L>H transition, L: flip on H>L transition, T: flip on any transition, -: no rule): "};
const PROGMEM char CONSOLE_TYPE_SAVE[]  = {"Confirm? (Y/N): "};
const PROGMEM char CONSOLE_CURRENT_CONFIG[]  = {"Current configuration:"};
const PROGMEM char CONSOLE_NEW_CONFIG[]  = {"New configuration:"};
const PROGMEM char CONSOLE_ERROR[]  = {"Error"};
const PROGMEM char CONSOLE_SAVED[]  = {"Saved"};

#ifndef SERIAL_PORT_MONITOR
#define SERIAL_PORT_MONITOR SERIAL_PORT_HARDWARE
#endif

const long SPEED_VALUE[] = {0, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};

byte consoleState = 0; // 0: wait for menu selection number; 1: speed; 2: parity; 3: address; 4: i/o rules; 5 confirm to save
byte opMode = 0; // 0: boot sequence, wait to enter console mode; 1: console mode; 2: Modbus slave mode
boolean validConfiguration;
unsigned long bootTimeMillis;
short spacesCounter = 0;
char consoleInputBuffer[7];

byte speedCurrent;
byte parityCurrent;
byte addressCurrent;
char rulesCurrent[7];
byte speedNew;
byte parityNew;
byte addressNew;
char rulesNew[7];

Stream *consolePort = NULL;

void setup() {
  bootTimeMillis = millis();
  
  SERIAL_PORT_HARDWARE.begin(9600);
  if ((int) &SERIAL_PORT_HARDWARE != (int) &SERIAL_PORT_MONITOR) {
    SERIAL_PORT_MONITOR.begin(9600);
  }

  // retrieve settings from EEPROM
  validConfiguration = getEEPROMConfig();

#ifdef IONO_MKR
  pinMode(PIN_TXEN, OUTPUT);
#endif
}

void loop() {
  if (opMode == 2) {
    IonoModbusRtuSlave.process();
    
  } else {
    if (consolePort == NULL) {
      if (SERIAL_PORT_HARDWARE.available()) {
        consolePort = &SERIAL_PORT_HARDWARE;
      } else if (SERIAL_PORT_MONITOR.available()) {
        consolePort = &SERIAL_PORT_MONITOR;
      }
    } else if (consolePort->available()) {
      int b = consolePort->read();
      if (opMode == 0) {
        if (b == ' ') {
          if (spacesCounter >= 4) {
#ifdef IONO_MKR
            digitalWrite(PIN_TXEN, HIGH);
#endif
            printConsoleMenu();
#ifdef IONO_MKR
            consolePort->flush();
            digitalWrite(PIN_TXEN, LOW);
#endif
            opMode = 1;
          } else {
            spacesCounter++;
          }
        } else if (validConfiguration) {
          startModbus();
        } else {
          consolePort = NULL;
        }
      } else { // opMode == 1
        serialConsole(b);
      }
    }
    
    if (validConfiguration && opMode == 0 && bootTimeMillis + BOOT_CONSOLE_TIMEOUT_MILLIS < millis()) {
      startModbus();
    }
  }
}

void startModbus() {
  SERIAL_PORT_HARDWARE.end();
  SERIAL_PORT_MONITOR.end();

  switch (parityCurrent) {
    case 2:
      IonoModbusRtuSlave.begin(addressCurrent, SPEED_VALUE[speedCurrent], SERIAL_8O1, DELAY);
      break;
    case 3:
      IonoModbusRtuSlave.begin(addressCurrent, SPEED_VALUE[speedCurrent], SERIAL_8N2, DELAY);
      break;
    default:
      IonoModbusRtuSlave.begin(addressCurrent, SPEED_VALUE[speedCurrent], SERIAL_8E1, DELAY);
  }

  if (rulesCurrent[0] != 0) {
    setLink(rulesCurrent[0], DI1, DO1);
    setLink(rulesCurrent[1], DI2, DO2);
    setLink(rulesCurrent[2], DI3, DO3);
    setLink(rulesCurrent[3], DI4, DO4);
#ifdef IONO_ARDUINO
    setLink(rulesCurrent[4], DI5, DO5);
    setLink(rulesCurrent[5], DI6, DO6);
#endif
  }

  opMode = 2;
}

void setLink(char rule, uint8_t dix, uint8_t dox) {
  switch (rule) {
    case 'F':
      Iono.linkDiDo(dix, dox, LINK_FOLLOW, DELAY);
      break;
    case 'I':
      Iono.linkDiDo(dix, dox, LINK_INVERT, DELAY);
      break;
    case 'T':
      Iono.linkDiDo(dix, dox, LINK_FLIP_T, DELAY);
      break;
    case 'H':
      Iono.linkDiDo(dix, dox, LINK_FLIP_H, DELAY);
      break;
    case 'L':
      Iono.linkDiDo(dix, dox, LINK_FLIP_L, DELAY);
      break;
    default:
      break;
  }
}

void serialConsole(int b) {
#ifdef IONO_MKR
  digitalWrite(PIN_TXEN, HIGH);
#endif
  delayMicroseconds(4000); // this is to let the console also work over the RS485 interface
  switch (consoleState) {
    case 0: // waiting for menu selection number
      switch (b) {
        case '0':
          consolePort->println((char)b);
          consolePort->println();
          printlnProgMemString(CONSOLE_CURRENT_CONFIG);
          printConfiguration(speedCurrent, parityCurrent, addressCurrent, rulesCurrent);
          printConsoleMenu();
          break;
        case '1':
          consoleState = 1;
          consoleInputBuffer[0] = 0;
          consolePort->println((char)b);
          consolePort->println();
          printProgMemString(CONSOLE_TYPE_SPEED);
          break;
        case '2':
          consoleState = 2;
          consoleInputBuffer[0] = 0;
          consolePort->println((char)b);
          consolePort->println();
          printProgMemString(CONSOLE_TYPE_PARITY);
          break;
        case '3':
          consoleState = 3;
          consoleInputBuffer[0] = 0;
          consolePort->println((char)b);
          consolePort->println();
          printProgMemString(CONSOLE_TYPE_ADDRESS);
          break;
        case '4':
          consoleState = 4;
          consoleInputBuffer[0] = 0;
          consolePort->println((char)b);
          consolePort->println();
          printProgMemString(CONSOLE_TYPE_MIRROR);
          break;
        case '5':
          consoleState = 5;
          consolePort->println((char)b);
          consolePort->println();
          printlnProgMemString(CONSOLE_NEW_CONFIG);
          printConfiguration((speedNew == 0) ? speedCurrent : speedNew, (parityNew == 0) ? parityCurrent : parityNew, (addressNew == 0) ? addressCurrent : addressNew, (rulesNew[0] == 0) ? rulesCurrent : rulesNew);
          printProgMemString(CONSOLE_TYPE_SAVE);
          break;
      }
      break;
    case 1: // speed
      if (numberEdit(consoleInputBuffer, &speedNew, b, 1, 1, 8)) {
        consoleState = 0;
        consolePort->println();
        printConsoleMenu();
      }
      break;
    case 2: // parity
      if (numberEdit(consoleInputBuffer, &parityNew, b, 1, 1, 3)) {
        consoleState = 0;
        consolePort->println();
        printConsoleMenu();
      }
      break;
    case 3: // address
      if (numberEdit(consoleInputBuffer, &addressNew, b, 3, 1, 247)) {
        consoleState = 0;
        consolePort->println();
        printConsoleMenu();
      }
      break;
    case 4: // rules
      if (rulesEdit(consoleInputBuffer, rulesNew, b, 6)) {
        consoleState = 0;
        consolePort->println();
        printConsoleMenu();
      }
      break;
    case 5: // confirm to save
      switch (b) {
        case 'Y':
        case 'y':
          consoleState = 0;
          consolePort->println('Y');
          if (saveConfig()) {
            printlnProgMemString(CONSOLE_SAVED);
            delay(1000);
            softReset();
          } else {
            printlnProgMemString(CONSOLE_ERROR);
          }
          printConsoleMenu();
          break;
        case 'N':
        case 'n':
          consoleState = 0;
          consolePort->println('N');
          consolePort->println();
          printConsoleMenu();
          break;
      }
      break;
    default:
      break;
  }
#ifdef IONO_MKR
  consolePort->flush();
  digitalWrite(PIN_TXEN, LOW);
#endif
}

boolean saveConfig() {
  return writeEepromConfig((speedNew == 0) ? speedCurrent : speedNew, (parityNew == 0) ? parityCurrent : parityNew, (addressNew == 0) ? addressCurrent : addressNew, (rulesNew[0] == 0) ? rulesCurrent : rulesNew);
}

boolean writeEepromConfig(byte speed, byte parity, byte address, char *rules) {
  byte checksum = 7;
  if (speed != 0 && parity != 0 && address != 0) {
    EEPROM.write(0, speed);
    checksum ^= speed;
    EEPROM.write(1, parity);
    checksum ^= parity;
    EEPROM.write(2, address);
    checksum ^= address;
    for (int a = 0; a < 6; a++) {
      EEPROM.write(a + 3, rules[a]);
      checksum ^= rules[a];
    }
    EEPROM.write(9, checksum);
#ifdef ARDUINO_ARCH_SAMD
    EEPROM.commit();
#endif
    return true;
  } else {
    return false;
  }
}

boolean readEepromConfig(byte *speedp, byte *parityp, byte *addressp, char *rulesp) {
  byte checksum = 7;

#ifdef ARDUINO_ARCH_SAMD
  if (!EEPROM.isValid()) {
    return false;
  }
#endif
  *speedp = EEPROM.read(0);
  checksum ^= *speedp;
  *parityp = EEPROM.read(1);
  checksum ^= *parityp;
  *addressp = EEPROM.read(2);
  checksum ^= *addressp;
  for (int a = 0; a < 6; a++) {
    rulesp[a] = EEPROM.read(a + 3);
    checksum ^= rulesp[a];
  }
  return (EEPROM.read(9) == checksum);
}

boolean getEEPROMConfig() {
  if (!readEepromConfig(&speedCurrent, &parityCurrent, &addressCurrent, rulesCurrent)) {
    speedCurrent = 0;
    parityCurrent = 0;
    addressCurrent = 0;
    rulesCurrent[0] = 0;
  }
  return (speedCurrent != 0 && parityCurrent != 0 && addressCurrent != 0);
}

void softReset() {
#ifdef ARDUINO_ARCH_SAMD
  NVIC_SystemReset();
#else
  asm volatile ("  jmp 0");
#endif
}

void printlnProgMemString(const char* s) {
  printProgMemString(s);
  consolePort->println();
}

void printProgMemString(const char* s) {
  int len = strlen_P(s);
  for (int k = 0; k < len; k++) {
    consolePort->print((char)pgm_read_byte_near(s + k));
  }
}

void printConsoleMenu() {
  consolePort->println();
  printlnProgMemString(CONSOLE_MENU_HEADER);
  for (int i = 0; i <= 5; i++) {
    consolePort->print(i);
    consolePort->print(". ");
    switch (i) {
      case 0:
        printlnProgMemString(CONSOLE_MENU_CURRENT_CONFIG);
        break;
      case 1:
        printlnProgMemString(CONSOLE_MENU_SPEED);
        break;
      case 2:
        printlnProgMemString(CONSOLE_MENU_PARITY);
        break;
      case 3:
        printlnProgMemString(CONSOLE_MENU_ADDRESS);
        break;
      case 4:
        printlnProgMemString(CONSOLE_MENU_MIRROR);
        break;
      case 5:
        printlnProgMemString(CONSOLE_MENU_SAVE);
        break;
    }
  }
  printProgMemString(CONSOLE_MENU_TYPE);
}

void printConfiguration(byte speed, byte parity, byte address, char *rules) {
  char s[] = ": ";
  printProgMemString(CONSOLE_MENU_SPEED);
  consolePort->print(s);
  if (speed == 0) {
    consolePort->println();
  } else {
    consolePort->println(SPEED_VALUE[speed]);
  }
  printProgMemString(CONSOLE_MENU_PARITY);
  consolePort->print(s);
  switch (parity) {
    case 1:
      consolePort->print("Even");
      break;
    case 2:
      consolePort->print("Odd");
      break;
    case 3:
      consolePort->print("None");
      break;
  }
  consolePort->println();
  printProgMemString(CONSOLE_MENU_ADDRESS);
  consolePort->print(s);
  if (address == 0) {
    consolePort->println();
  } else {
    consolePort->println(address);
  }
  printProgMemString(CONSOLE_MENU_MIRROR);
  consolePort->print(s);
  if (rules[0] == 0) {
    consolePort->println();
  } else {
    consolePort->println(rules);
  }
}

boolean rulesEdit(char *buffer, char *value, int c, int size) {
  int i;
  switch (c) {
    case 8: case 127: // backspace
      i = strlen(buffer);
      if (i > 0) {
        consolePort->print('\b');
        consolePort->print(' ');
        consolePort->print('\b');
        buffer[i - 1] = 0;
      }
      break;
    case 10: // newline
    case 13: // enter
      if (strlen(buffer) == 6) {
        strcpy(value, buffer);
        return true;
      } else {
        return false;
      }
      break;
    default:
      if (strlen(buffer) < size) {
        if (c >= 'a') {
          c -= 32;
        }
        if (c == 'F' || c == 'I' || c == 'H' || c == 'L' || c == 'T' || c == '-') {
          consolePort->print(' ');
          consolePort->print('\b');
          consolePort->print((char)c);
          strcat_c(buffer, c);
        }
      }
      break;
  }
  return false;
}

boolean numberEdit(char *buffer, byte *value, int c, int length, long min, long max) {
  int i;
  long v;
  switch (c) {
    case 8: case 127: // backspace
      i = strlen(buffer);
      if (i > 0) {
        consolePort->print('\b');
        consolePort->print(' ');
        consolePort->print('\b');
        buffer[i - 1] = 0;
      }
      break;
    case 10: // newline
    case 13: // enter
      v = strtol(buffer, NULL, 10);
      if (v >= min && v <= max) {
        *value = (byte)v;
        return true;
      } else {
        return false;
      }
      break;
    default:
      if (strlen(buffer) < length) {
        if (c >= '0' && c <= '9') {
          consolePort->print(' ');
          consolePort->print('\b');
          consolePort->print((char)c);
          strcat_c(buffer, c);
        }
      }
      break;
  }
  return false;
}

void strcat_c(char *s, char c) {
  for (; *s; s++);
  *s++ = c;
  *s++ = 0;
}

