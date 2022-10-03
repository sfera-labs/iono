/*
  IonoMkrLoRaWAN.cpp

    Copyright (C) 2019-2022 Sfera Labs S.r.l. - All rights reserved.

    For information, see:
    https://www.sferalabs.cc/

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any nlater version.
  See file LICENSE.txt for further informations on licesing terms.
*/

#include <Iono.h>
#include "MKRWAN.h"
#include "SerialConfig.h"
#include "Watchdog.h"
#include "CayenneLPP.h"

#define DEBOUNCE_MS  25

LoRaModem modem;
CayenneLPP lpp(100);

uint8_t in1;
uint8_t in2;
uint8_t in3;
uint8_t in4;
uint8_t rcvBuf[16];
float lastSentIn[] = {-1, -1, -1, -1, -1, -1};
uint16_t lastSentCount[] = {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff};
uint16_t valCount[] = {0, 0, 0, 0, 0, 0};
uint8_t lastSentOut[] = {0xff, 0xff, 0xff, 0xff};
float lastSentAO1 = -1;
unsigned long lastUpdateSendTs;
unsigned long lastFullStateSendTs;
unsigned long lastHeartbeatSendTs;
bool needToSend = false;
bool initialized = false;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  SerialConfig.setup();
  while (!SerialConfig.isConfigured) {
    SerialConfig.process();
  }
  
  if (!initialize()) {
    delay(4000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
    NVIC_SystemReset();
  }
  
  lastUpdateSendTs = lastFullStateSendTs = lastHeartbeatSendTs = millis();
  digitalWrite(LED_BUILTIN, LOW);

  // Sleep random time up to 7 seconds
  uint32_t seed = 0;
  for (int i = 0; i < 32; i++) {
    seed <<= 1;
    seed |= analogRead(9) & 1;
  }
  randomSeed(seed);
  delay(random(7000));
}

void checkDataRate(bool force = false) {
  static unsigned long lastTs = 0;
  int dr;
  unsigned long now = millis();
  if (force || now - lastTs >= 5000) {
    lastTs = now;
    dr = modem.getDataRate();
    if (dr >= 0) {
      if (dr != SerialConfig.dataRate) {
        for (int i = 0; i < 3; i++) {
          if (modem.dataRate(SerialConfig.dataRate)) {
            break;
          }
        }
      }
    }
  }
}

void checkFrameCounters() {
  static unsigned long lastUpTs = 0;
  static unsigned long lastDownTs = 0;
  unsigned long now = millis();
  
  if (now - lastUpTs >= 300000) {
    int64_t fCntUp = modem.getFCU();
    if (fCntUp >= 0) {
      if (fCntUp > SerialConfig.fCntUp + 50) {
        SerialConfig.writeFCntUp(fCntUp + 50);
        SerialConfig.fCntUp = fCntUp;
      }
      lastUpTs = now;
    }
  }

  if (now - lastDownTs >= 300000) {
    int64_t fCntDown = modem.getFCD();
    if (fCntDown >= 0) {
      if (fCntDown > SerialConfig.fCntDown + 20) {
        SerialConfig.writeFCntDown(fCntDown);
        SerialConfig.fCntDown = fCntDown;
      }
      lastDownTs = now;
    }
  }
}

void loop() {
  Iono.process();

  unsigned long now = millis();
  if (now - lastFullStateSendTs >= 15 * 60000 || now - lastUpdateSendTs >= 7 * 60000) {
    if (sendState(true)) {
      lastFullStateSendTs = now;
    }
  } else if (needToSend || now - lastHeartbeatSendTs >= 3 * 60000) {
    sendState(false);
  }

  while (modem.available()) {
    if (modem.read(rcvBuf, 4) == 4 && rcvBuf[3] == 0xff) {
      switch (rcvBuf[0]) {
        case 101: Iono.write(DO1, rcvBuf[2] == 0 ? LOW : HIGH); lastSentOut[0] = 0xff; break;
        case 102: Iono.write(DO2, rcvBuf[2] == 0 ? LOW : HIGH); lastSentOut[1] = 0xff; break;
        case 103: Iono.write(DO3, rcvBuf[2] == 0 ? LOW : HIGH); lastSentOut[2] = 0xff; break;
        case 104: Iono.write(DO4, rcvBuf[2] == 0 ? LOW : HIGH); lastSentOut[3] = 0xff; break;
        case 201: Iono.write(AO1, (((rcvBuf[1] & 0xff) << 8) + rcvBuf[2]) / 100.0); lastSentAO1 = -1; break;
      }
      needToSend = true;
    }
  }

  checkDataRate();

  checkFrameCounters();
  
  if (SerialConfig.isAvailable) {
    SerialConfig.process();
  }
  
  Watchdog.clear();
}

bool send(uint8_t* data, size_t len) {
  modem.beginPacket();
  modem.write(data, len);
  return modem.endPacket(false) > 0;
}

bool sendState(bool sendAll) {
  static unsigned long lastTs = 0;
  float valIn[6];
  uint8_t valOut[4];
  float valAO1;

  unsigned long now = millis();
  if (now - lastTs < 1500) {
    return false;
  }
  lastTs = now - random(500);

  digitalWrite(LED_BUILTIN, HIGH);
  
  valIn[0] = Iono.read(in1);
  valIn[1] = Iono.read(in2);
  valIn[2] = Iono.read(in3);
  valIn[3] = Iono.read(in4);
  valIn[4] = Iono.read(DI5);
  valIn[5] = Iono.read(DI6);
  valOut[0] = (uint8_t) Iono.read(DO1);
  valOut[1] = (uint8_t) Iono.read(DO2);
  valOut[2] = (uint8_t) Iono.read(DO3);
  valOut[3] = (uint8_t) Iono.read(DO4);
  valAO1 = Iono.read(AO1);

  needToSend = false;
  
  lpp.reset();

  lpp.addLuminosity(99, now / 1000);

  for (int i = 0; i < 6; i++) {
    if (sendAll || lastSentIn[i] != valIn[i]) {
      switch (SerialConfig.modes[i]) {
        case 'D': needToSend = true; lpp.addDigitalInput(i + 1, (uint8_t) valIn[i]); break;
        case 'V': needToSend = true; lpp.addAnalogInput(i + 11, valIn[i]); break;
        case 'I': needToSend = true; lpp.addAnalogInput(i + 21, valIn[i]); break;
        default : break;
      }
    }
  }

  for (int i = 0; i < 6; i++) {
    if (SerialConfig.modes[i] == 'D') {
      if (sendAll || lastSentCount[i] != valCount[i]) {
        needToSend = true;
        lpp.addAnalogInput(i + 51, valCount[i]);
      }
    }
  }

  for (int i = 0; i < 4; i++) {
    if (sendAll || lastSentOut[i] != valOut[i]) {
      needToSend = true;
      lpp.addDigitalOutput(i + 101, valOut[i]);
    }
  }

  if (sendAll || lastSentAO1 != valAO1) {
    needToSend = true;
    lpp.addAnalogOutput(201, valAO1);
  }

  if (send(lpp.getBuffer(), lpp.getSize())) {
    for (int i = 0; i < 6; i++) {
      lastSentIn[i] = valIn[i];
    }
    for (int i = 0; i < 6; i++) {
      lastSentCount[i] = valCount[i];
    }
    for (int i = 0; i < 4; i++) {
      lastSentOut[i] = valOut[i];
    }
    lastSentAO1 = valAO1;
    if (needToSend) {
      lastUpdateSendTs = now;
      needToSend = false;
    }
    lastHeartbeatSendTs = now;
  }
    
  digitalWrite(LED_BUILTIN, LOW);

  return !needToSend;
}

void inputsCallback(uint8_t pin, float value) {
  int idx;
  if (value == HIGH) {
    switch (pin) {
      case DI1: idx = 0; break;
      case DI2: idx = 1; break;
      case DI3: idx = 2; break;
      case DI4: idx = 3; break;
      case DI5: idx = 4; break;
      case DI6: idx = 5; break;
      default: idx = -1; break;
    }
    if (idx >= 0) {
      if (valCount[idx] < 327) {
        valCount[idx]++;
      } else {
        valCount[idx] = 0;
      }
    }
  }
  
  needToSend = true;
}

bool initialize() {
  Watchdog.setup();
  
  if (!modem.begin(SerialConfig.band)) {
    return false;
  }
  if (!modem.joinABP(SerialConfig.devAddr, SerialConfig.nwkSKey, SerialConfig.appSKey)) {
    return false;
  }
  if (!modem.dutyCycle(true)) {
    return false;
  }
  if (!modem.setADR(true)) {
    return false;
  }
  if (!modem.configureClass(CLASS_C)) {
    return false;
  }
  if (!modem.setFCU(SerialConfig.fCntUp)) {
    return false;
  }
  if (!modem.setFCD(SerialConfig.fCntDown)) {
    return false;
  }
  checkDataRate(true);

  Iono.subscribeDigital(DO1, 0, &inputsCallback);
  Iono.subscribeDigital(DO2, 0, &inputsCallback);
  Iono.subscribeDigital(DO3, 0, &inputsCallback);
  Iono.subscribeDigital(DO4, 0, &inputsCallback);
  
  Iono.subscribeAnalog(AO1, 0, 0, &inputsCallback);
  
  subscribeMultimode(SerialConfig.modes[0], &in1, DI1, AV1, AI1);
  subscribeMultimode(SerialConfig.modes[1], &in2, DI2, AV2, AI2);
  subscribeMultimode(SerialConfig.modes[2], &in3, DI3, AV3, AI3);
  subscribeMultimode(SerialConfig.modes[3], &in4, DI4, AV4, AI4);
  subscribeMultimode(SerialConfig.modes[4], NULL, DI5, 0, 0);
  subscribeMultimode(SerialConfig.modes[5], NULL, DI6, 0, 0);

  if (SerialConfig.rules[0] != '\0') {
    setLink(SerialConfig.modes[0], SerialConfig.rules[0], DI1, DO1);
    setLink(SerialConfig.modes[1], SerialConfig.rules[1], DI2, DO2);
    setLink(SerialConfig.modes[2], SerialConfig.rules[2], DI3, DO3);
    setLink(SerialConfig.modes[3], SerialConfig.rules[3], DI4, DO4);
  }

  return true;
}

void subscribeMultimode(char mode, uint8_t* inx, uint8_t dix, uint8_t avx, uint8_t aix) {
  switch (mode) {
    case 'D':
      if (inx != NULL) {
        *inx = dix;
      }
      Iono.subscribeDigital(dix, DEBOUNCE_MS, &inputsCallback);
      break;
    case 'V':
      if (inx != NULL) {
        *inx = avx;
      }
      Iono.subscribeAnalog(avx, DEBOUNCE_MS, 0.1, &inputsCallback);
      break;
    case 'I':
      if (inx != NULL) {
        *inx = aix;
      }
      Iono.subscribeAnalog(aix, DEBOUNCE_MS, 0.1, &inputsCallback);
      break;
    default:
      break;
  }
}

void setLink(char mode, char rule, uint8_t dix, uint8_t dox) {
  if (mode == 'V' || mode == 'I') {
    return;
  }
  switch (rule) {
    case 'F':
      Iono.linkDiDo(dix, dox, LINK_FOLLOW, DEBOUNCE_MS);
      break;
    case 'I':
      Iono.linkDiDo(dix, dox, LINK_INVERT, DEBOUNCE_MS);
      break;
    case 'T':
      Iono.linkDiDo(dix, dox, LINK_FLIP_T, DEBOUNCE_MS);
      break;
    case 'H':
      Iono.linkDiDo(dix, dox, LINK_FLIP_H, DEBOUNCE_MS);
      break;
    case 'L':
      Iono.linkDiDo(dix, dox, LINK_FLIP_L, DEBOUNCE_MS);
      break;
    default:
      break;
  }
}
