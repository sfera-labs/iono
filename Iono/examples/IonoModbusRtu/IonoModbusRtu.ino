/* 
  IonoModbusRtu.cpp - A Modbus RTU slave for Iono Uno - Version 2.0
  
    Copyright (C) 2016-2017 Sfera Labs S.r.l. - All rights reserved.
    
    For information, see the iono web site:
    http://www.sferalabs.cc/iono-arduino
  
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#ifdef ARDUINO_SAMD_ZERO
#include <FlashAsEEPROM.h>
#include <FlashStorage.h>
#else
#include <EEPROM.h>
#endif

#include <Iono.h>

#define DELAY  25                           // the debounce delay in milliseconds
#define BOOT_CONSOLE_TIMEOUT_MILLIS 15000   // if 5 consecutive spaces are received within this time interval after boot, enter console mode

#ifdef ARDUINO_SAMD_ZERO
const PROGMEM char CONSOLE_MENU_HEADER[]  = {"Sfera Labs Iono Zero (2.0) - Modbus RTU slave configuration menu"};
#else
const PROGMEM char CONSOLE_MENU_HEADER[]  = {"Sfera Labs Iono Uno (2.0) - Modbus RTU slave configuration menu"};
#endif
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

// Table of CRC values for high–order byte
static byte auchCRCHi[] = {
   0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
   0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
   0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
   0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
   0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
   0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
   0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
   0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
   0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
   0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
   0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
   0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
   0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
   0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
   0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
   0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
   0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
   0x40
};

// Table of CRC values for low–order byte
static byte auchCRCLo[] = {
   0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
   0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
   0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
   0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
   0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
   0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
   0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
   0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
   0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
   0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
   0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
   0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
   0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
   0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
   0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
   0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
   0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
   0x40
};

const long SPEED_VALUE[] = {0, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};

#ifdef ARDUINO_SAMD_ZERO
HardwareSerial& consolePort = Serial;
HardwareSerial& rs485Port = Serial1;
#else
HardwareSerial& consolePort = Serial;
HardwareSerial& rs485Port = Serial;
#endif

byte inputFrameState; // 0: waiting for new frame, 1: receiving frame with matching address, 2: receiving frame with non-matching address (to be skipped), 3: error
unsigned long lastFrameCharacterTimeMicros = 0;
unsigned long nowMicros;
unsigned long frameCharacterInterval;
byte consoleState = 0; // 0: wait for menu selection number; 1: speed; 2: parity; 3: address; 4: i/o rules; 5 confirm to save
byte opMode = 0; // 0: boot sequence, wait to enter console mode; 1: console mode; 2: Modbus slave mode
boolean validConfiguration;
unsigned long bootTimeMillis;
short spacesCounter = 0;
char consoleInputBuffer[7];

unsigned short analogOutValue = 0; // 0-10000 mV

short values[6];            // current valid state for digital inputs
short lastvalues[6];        // values read in last loop
long times[6];              // last change timestamp
unsigned short counters[6]; // digital inputs counter

byte speedCurrent;
byte parityCurrent;
byte addressCurrent;
char rulesCurrent[7];
byte speedNew;
byte parityNew;
byte addressNew;
char rulesNew[7];

unsigned long t15micros;
unsigned long t35micros;

int messageIndex;
byte message[16];
byte responseMessage[16];

void setup() {
  bootTimeMillis = millis();
  consolePort.begin(9600);

  // retrieve settings from EEPROM
  validConfiguration = getEEPROMConfig();

  // set initial status of digital inputs to unknown
  for (int i = 0; i < sizeof(values) / sizeof(short); i++) {
    values[i] = -1;
    lastvalues[i] = -1;
    times[i] = 0;
  }
}

void loop() {
  debounce(); // digital inputs debouncer and counter

  if (opMode == 2) {
    if (rs485Port.available()) {
      int b = rs485Port.read();
      inputProcessor(b);
    } else if (validConfiguration) {
      nowMicros = micros();
      frameCharacterInterval = nowMicros - lastFrameCharacterTimeMicros;
      if (frameCharacterInterval > t35micros) {
        if (inputFrameState == 1) {
         // process frame
         if (crc(message, messageIndex - 2) == (unsigned short)(message[messageIndex - 1] << 8 | message[messageIndex - 2])) {
          byte pdulength = processPdu(message + 1, responseMessage + 1);
          responseMessage[0] = addressCurrent;
          unsigned short c = crc(responseMessage, pdulength + 1);
          responseMessage[pdulength + 1] = c & 0xff; // low CRC byte firs
          responseMessage[pdulength + 2] = (c >> 8) & 0xff;
          rs485Port.write(responseMessage, pdulength + 3);
         }
        }
        inputFrameState = 0;
      }
    } 
  } else {
    if (consolePort.available()) {
      int b = consolePort.read();
      switch (opMode) {
        case 0:
          if (b == ' ') {
            if (spacesCounter >= 4) {
              printConsoleMenu();
              opMode = 1;
            } else {
              spacesCounter++;
            }
          } else if (validConfiguration) {
            opMode = 2;
            setSerial();
          }
          break;
        case 1:
          serialConsole(b);
          break;
      }
    } else if (validConfiguration && opMode == 0 && bootTimeMillis + BOOT_CONSOLE_TIMEOUT_MILLIS < millis()) {
      opMode = 2;
      setSerial();
    } 
  }
}

void setSerial() {
  if (SPEED_VALUE[speedCurrent] <= 19200) {
    t15micros = 1500000 * 11 / SPEED_VALUE[speedCurrent];
    t35micros = 3500000 * 11 / SPEED_VALUE[speedCurrent];
  } else {
    t15micros = 750;
    t35micros = 1750;
  }
  inputFrameState = 0;

  consolePort.end();

  switch (parityCurrent) {
    case 1:
      rs485Port.begin(SPEED_VALUE[speedCurrent], SERIAL_8E1);
      break;
    case 2:
      rs485Port.begin(SPEED_VALUE[speedCurrent], SERIAL_8O1);
      break;
    case 3:
      rs485Port.begin(SPEED_VALUE[speedCurrent], SERIAL_8N2);
      break;
  }
}

void inputProcessor(byte b) {
  nowMicros = micros();
  frameCharacterInterval = nowMicros - lastFrameCharacterTimeMicros;
  lastFrameCharacterTimeMicros = nowMicros;
  switch (inputFrameState) {
    case 0:
      if (b == addressCurrent) {
        inputFrameState = 1;
        message[0] = b;
        messageIndex = 1;
      } else {
        inputFrameState = 2;
      }
      break;
    case 1:
      if (frameCharacterInterval > t15micros) {
        inputFrameState = 3;
      } else if (messageIndex < sizeof(message)) {
        message[messageIndex++] = b;
      } else {
        inputFrameState = 3;
      }
      break;
    case 2:
      if (frameCharacterInterval > t15micros) {
        inputFrameState = 3;
      }
      break;
  }
}

unsigned short crc(byte *message, byte length) {
  byte uchCRCHi = 0xFF;
  byte uchCRCLo = 0xFF;
  byte uIndex;

  while (length--) {
    uIndex = uchCRCLo ^ *message++;
    uchCRCLo = uchCRCHi ^ auchCRCHi[uIndex];
    uchCRCHi = auchCRCLo[uIndex];
  }
  return (uchCRCHi << 8 | uchCRCLo) ;
}

byte processPdu(byte *pdu, byte *rpdu) {
  int start, quantity;
  byte pdulength;
  switch (pdu[0]) {
    case 1: // read coils
      // read status of output relays (DO1-DO6), Modbus address 1-6
      if (pdu[1] == 0 && pdu[3] == 0 && pdu[2] > 0 && pdu[4] > 0 && pdu[2] + pdu[4] <= 7) {
        pdulength = 3;
        rpdu[0] = rpdu[1] = 1;
        rpdu[2] = 0;
        for (int i = pdu[2] + pdu[4] - 1; i >= pdu[2]; i--) {
          rpdu[2] <<= 1;
          if (Iono.read(indexToDigitalOutput(i)) == HIGH) {
            rpdu[2] += 1;
          }
        }
      } else {
        pdulength = 2;
        rpdu[0] = 0x81;
        rpdu[1] = 2; // illegal data address
      }
      break;
    case 2: // read discrete inputs
      // read status of digital inputs (DI1-DI6), Modbus address 101-106 (with de-bouce) and 111-116 (no de-bouce)
      if (pdu[1] == 0 && pdu[3] == 0 && (pdu[2] > 100 && pdu[2] < 107 || pdu[2] > 110 && pdu[2] < 117) && pdu[4] > 0 && pdu[4] < 7) {
        pdulength = 3;
        rpdu[0] = 2;
        rpdu[1] = 1;
        rpdu[2] = 0;
        for (int i = pdu[4]; i > 0 ; i--) {
          rpdu[2] <<= 1;
          if (pdu[2] > 110) { // no de-bouce
            if (Iono.read(indexToDigitalInput(i + pdu[2] - 111)) == HIGH) {
              rpdu[2] += 1;
            }
          } else { // de-bounce
            if (values[i + pdu[2] - 102] == 1) {
              rpdu[2] += 1;
            }
          }
        }
      } else {
        pdulength = 2;
        rpdu[0] = 0x82;
        rpdu[1] = 2; // illegal data address
      }
      break;
    case 3: // read holding registers
      // read status of analog output (AO1), Modbus address 601
      if (pdu[1] == 2 && pdu[2] == 89 && pdu[3] == 0 && pdu[4] == 1) {
        pdulength = 4;
        rpdu[0] = 3;
        rpdu[1] = 2;
        rpdu[2] = (byte)(analogOutValue >> 8);
        rpdu[3] = (byte)(analogOutValue & 0xff);
      } else {
        pdulength = 2;
        rpdu[0] = 0x83;
        rpdu[1] = 2; // illegal data address
      }
      break;
    case 4: // read input registers
      start = (pdu[1] << 8) + pdu[2];
      quantity = (pdu[3] << 8) + pdu[4];
      if (start >= 201 && start <= 204 && start + quantity <= 205) {
        // read status of analog voltage inputs (AV1-AV4), Modbus address 201-204
        pdulength = 2 * quantity + 2;
        rpdu[0] = 4;
        rpdu[1] = 2 * quantity;
        int v;
        for (int i = 1; i <= quantity; i++) {
          v = Iono.read(indexToVoltageInput(i + start - 201)) * 1000;
          rpdu[i * 2] = (byte)(v >> 8);
          rpdu[1 + i * 2] = (byte)(v & 0xff);
        }
      } else if (start >= 301 && start <= 304 && start + quantity <= 305) {
        // read status of analog current inputs (AI1-AI4), Modbus address 301-304
        pdulength = 2 * quantity + 2;
        rpdu[0] = 4;
        rpdu[1] = 2 * quantity;
        int v;
        for (int i = 1; i <= quantity; i++) {
          v = Iono.read(indexToCurrentInput(i + start - 301)) * 1000;
          rpdu[i * 2] = (byte)(v >> 8);
          rpdu[1 + i * 2] = (byte)(v & 0xff);
        }
      } else if (start >= 1001 && start <= 1006 && start + quantity <= 1007) {
        // read status of digital inputs (DI1-DI6) counters, Modbus address 1001-1006
        pdulength = 2 * quantity + 2;
        rpdu[0] = 4;
        rpdu[1] = 2 * quantity;
        int v;
        for (int i = 1; i <= quantity; i++) {
          v = counters[i + start - 1002];
          rpdu[i * 2] = (byte)(v >> 8);
          rpdu[1 + i * 2] = (byte)(v & 0xff);
        }
      } else {
        pdulength = 2;
        rpdu[0] = 0x84;
        rpdu[1] = 2; // illegal data address
      }
      break;
    case 5: // write single coil
      // command of single output relay (DO1-DO6), Modbus address 1-6
      if (pdu[1] == 0 && pdu[2] > 0 && pdu[2] <= 6) {
        if (pdu[3] == 0 && pdu[4] == 0) {
          Iono.write(indexToDigitalOutput(pdu[2]), LOW);
          pdulength = 5;
          for (int i = 0; i < 5; i++) {
            rpdu[i] = pdu[i];
          }
        } else if (pdu[3] == 0xff && pdu[4] == 0) {
          Iono.write(indexToDigitalOutput(pdu[2]), HIGH);
          pdulength = 5;
          for (int i = 0; i < 5; i++) {
            rpdu[i] = pdu[i];
          }
        } else {
          pdulength = 2;
          rpdu[0] = 0x81;
          rpdu[1] = 3; // illegal data value
        }
      } else {
        pdulength = 2;
        rpdu[0] = 0x81;
        rpdu[1] = 2; // illegal data address
      }
      break;
    case 6: // write single register
      // command of analog output (AO1), Modbus address 601, 0-10000 mV
      if (pdu[1] == 2 && pdu[2] == 89) {
        int v = (pdu[3] << 8) + pdu[4];
        if (v >= 0 && v <= 10000) {
          analogOutValue = v;
          Iono.write(AO1, analogOutValue / 1000.0);
          pdulength = 5;
          for (int i = 0; i < 5; i++) {
            rpdu[i] = pdu[i];
          }
        } else {
          pdulength = 2;
          rpdu[0] = 0x86;
          rpdu[1] = 3; // illegal data value
        }
      } else {
        pdulength = 2;
        rpdu[0] = 0x86;
        rpdu[1] = 2; // illegal data address
      }
      break;
    case 15: // write multiple coils
      // command of multiple output relays (DO1-DO6), Modbus address 1-6
      if (pdu[1] == 0 && pdu[2] > 0 && pdu[3] == 0 && pdu[4] > 0 && pdu[2] + pdu[4] <= 7) {
        if (pdu[5] == 1) {
          for (int i = pdu[2]; i < pdu[2] + pdu[4]; i++) {
            Iono.write(indexToDigitalOutput(i), ((pdu[6] & 1) == 0) ? LOW : HIGH);
            pdu[6] >>= 1;
          }
          pdulength = 5;
          for (int i = 0; i < 5; i++) {
            rpdu[i] = pdu[i];
          }
        } else {
          pdulength = 2;
          rpdu[0] = 0x8f;
          rpdu[1] = 3; // illegal data value
        }
      } else {
        pdulength = 2;
        rpdu[0] = 0x8f;
        rpdu[1] = 2; // illegal data address
      }
      break;
    default: // error
      pdulength = 2;
      rpdu[0] = pdu[0] | 0x80;
      rpdu[1] = 1; // illegal function
      break;
  }
  return pdulength;
}

void serialConsole(int b) {
  delayMicroseconds(4000); // this is to let the console also work over the RS485 interface
  switch (consoleState) {
    case 0: // waiting for menu selection number
      switch (b) {
        case '0':
          consolePort.println((char)b);
          consolePort.println();
          printlnProgMemString(CONSOLE_CURRENT_CONFIG);
          printConfiguration(speedCurrent, parityCurrent, addressCurrent, rulesCurrent);
          printConsoleMenu();
          break;
        case '1':
          consoleState = 1;
          consoleInputBuffer[0] = 0;
          consolePort.println((char)b);
          consolePort.println();
          printProgMemString(CONSOLE_TYPE_SPEED);
          break;
        case '2':
          consoleState = 2;
          consoleInputBuffer[0] = 0;
          consolePort.println((char)b);
          consolePort.println();
          printProgMemString(CONSOLE_TYPE_PARITY);
          break;
        case '3':
          consoleState = 3;
          consoleInputBuffer[0] = 0;
          consolePort.println((char)b);
          consolePort.println();
          printProgMemString(CONSOLE_TYPE_ADDRESS);
          break;
        case '4':
          consoleState = 4;
          consoleInputBuffer[0] = 0;
          consolePort.println((char)b);
          consolePort.println();
          printProgMemString(CONSOLE_TYPE_MIRROR);
          break;
        case '5':
          consoleState = 5;
          consolePort.println((char)b);
          consolePort.println();
          printlnProgMemString(CONSOLE_NEW_CONFIG);
          printConfiguration((speedNew == 0) ? speedCurrent : speedNew, (parityNew == 0) ? parityCurrent : parityNew, (addressNew == 0) ? addressCurrent : addressNew, (rulesNew[0] == 0) ? rulesCurrent : rulesNew);
          printProgMemString(CONSOLE_TYPE_SAVE);
          break;
      }
      break;
    case 1: // speed
      if (numberEdit(consoleInputBuffer, &speedNew, b, 1, 1, 8)) {
        consoleState = 0;
        consolePort.println();
        printConsoleMenu();
      }
      break;
    case 2: // parity
      if (numberEdit(consoleInputBuffer, &parityNew, b, 1, 1, 3)) {
        consoleState = 0;
        consolePort.println();
        printConsoleMenu();
      }
      break;
    case 3: // address
      if (numberEdit(consoleInputBuffer, &addressNew, b, 3, 1, 247)) {
        consoleState = 0;
        consolePort.println();
        printConsoleMenu();
      }
      break;
    case 4: // rules
      if (rulesEdit(consoleInputBuffer, rulesNew, b, 6)) {
        consoleState = 0;
        consolePort.println();
        printConsoleMenu();
      }
      break;
    case 5: // confirm to save
      switch (b) {
        case 'Y':
        case 'y':
          consoleState = 0;
          consolePort.println('Y');
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
          consolePort.println('N');
          consolePort.println();
          printConsoleMenu();
          break;
      }
      break;
    default:
      break;
  }
}

void debounce() {
  int value;
  boolean changed, initialize;
  for (int i = 0; i < sizeof(values) / sizeof(short); i++) {
    changed = false;
    initialize = false;
    value = (Iono.read(indexToDigitalInput(i + 1)) == HIGH) ? 1 : 0;
    if (value != lastvalues[i]) {
      times[i] = millis();
    }
    if ((millis() - times[i]) > DELAY) {
      if (values[i] == -1) {
        values[i] = value;
        changed = true;
        initialize = true;
      } else if (values[i] != value) {
        values[i] = value;
        changed = true;
        if (value == 1) {
          counters[i] = (counters[i] < 65535) ? counters[i] + 1 : 0;
        }
      }
    }

    if (changed) {
      switch (rulesCurrent[i]) {
        case 'F': // follow
          Iono.write(indexToDigitalOutput(i + 1), (value) ? HIGH : LOW);
          break;
        case 'I': // follow, inverted
          Iono.write(indexToDigitalOutput(i + 1), (value) ? LOW : HIGH);
          break;
        case 'H': // flip on low to high transition
          if (!initialize && value == 1) {
            Iono.flip(indexToDigitalOutput(i + 1));
          }
          break;
        case 'L': // flip on high to low transition
          if (!initialize && value == 0) {
            Iono.flip(indexToDigitalOutput(i + 1));
          }
          break;
        case 'T': // flip on any transition
          if (!initialize) {
            Iono.flip(indexToDigitalOutput(i + 1));
          }
          break;
      }
    }
    lastvalues[i] = value;
  }
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
#ifdef ARDUINO_SAMD_ZERO
    EEPROM.commit();
#endif
    return true;
  } else {
    return false;
  }
}

boolean readEepromConfig(byte *speedp, byte *parityp, byte *addressp, char *rulesp) {
  byte checksum = 7;
  int a = 0;

#ifdef ARDUINO_SAMD_ZERO
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
#ifdef ARDUINO_SAMD_ZERO
  NVIC_SystemReset();
#else
  asm volatile ("  jmp 0");
#endif

}

void printlnProgMemString(const char* s) {
  printProgMemString(s);
  consolePort.println();
}

void printProgMemString(const char* s) {
  int len = strlen_P(s);
  for (int k = 0; k < len; k++) {
    consolePort.print((char)pgm_read_byte_near(s + k));
  }
}

void printConsoleMenu() {
  consolePort.println();
  printlnProgMemString(CONSOLE_MENU_HEADER);
  for (int i = 0; i <= 5; i++) {
    consolePort.print(i);
    consolePort.print(". ");
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
  consolePort.print(s);
  if (speed == 0) {
    consolePort.println();
  } else {
    consolePort.println(SPEED_VALUE[speed]);
  }
  printProgMemString(CONSOLE_MENU_PARITY);
  consolePort.print(s);
  switch (parity) {
    case 1:
      consolePort.print("Even");
      break;
    case 2:
      consolePort.print("Odd");
      break;
    case 3:
      consolePort.print("None");
      break;
  }
  consolePort.println();
  printProgMemString(CONSOLE_MENU_ADDRESS);
  consolePort.print(s);
  if (address == 0) {
    consolePort.println();
  } else {
    consolePort.println(address);
  }
  printProgMemString(CONSOLE_MENU_MIRROR);
  consolePort.print(s);
  if (rules[0] == 0) {
    consolePort.println();
  } else {
    consolePort.println(rules);
  }
}

boolean rulesEdit(char *buffer, char *value, int c, int size) {
  int i;
  switch (c) {
    case 8: case 127: // backspace
      i = strlen(buffer);
      if (i > 0) {
        consolePort.print('\b');
        consolePort.print(' ');
        consolePort.print('\b');
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
          consolePort.print((char)c);
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
        consolePort.print('\b');
        consolePort.print(' ');
        consolePort.print('\b');
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
          consolePort.print((char)c);
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

int indexToCurrentInput(int i) {
  switch (i) {
    case 1:
      return AI1;
    case 2:
      return AI2;
    case 3:
      return AI3;
    case 4:
      return AI4;
    default:
      return -1;
  }
}

int indexToVoltageInput(int i) {
  switch (i) {
    case 1:
      return AV1;
    case 2:
      return AV2;
    case 3:
      return AV3;
    case 4:
      return AV4;
    default:
      return -1;
  }
}

int indexToDigitalInput(int i) {
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
    default:
      return -1;
  }
}

int indexToDigitalOutput(int i) {
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
    default:
      return -1;
  }
}
