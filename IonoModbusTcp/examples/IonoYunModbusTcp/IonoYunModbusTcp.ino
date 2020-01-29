/*
  IonoModbusTcp.cpp - A Modbus TCP server for Iono Arduino with Arduino YUN

    Copyright (C) 2018 Sfera Labs S.r.l. - All rights reserved.

    For information, see:
    http://www.sferalabs.cc/iono

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#include <Iono.h>
#include <Bridge.h>
#include <BridgeServer.h>
#include <BridgeClient.h>

#define DELAY  50            // the debounce delay in milliseconds
#define CLIENTS_MAX 5
#define CLIENT_TIMEOUT 30000

short values[6];            // current valid state for digital inputs
short lastvalues[6];        // values read in last loop
long times[6];              // last change timestamp
unsigned short counters[6]; // digital inputs counter

BridgeServer server(502);   // Modbus TCP server listening on port 502

BridgeClient clients[CLIENTS_MAX];

int mbapindex[CLIENTS_MAX]; // set to -1 when done reading MBAP header
int pduindex[CLIENTS_MAX];
int pdulength[CLIENTS_MAX];

byte mbap[CLIENTS_MAX][7];
byte pdu[CLIENTS_MAX][16];
byte rpdu[CLIENTS_MAX][16];

unsigned long lastrequest[CLIENTS_MAX];

void setup() {
  Bridge.begin();

  server.noListenOnLocalhost();
  server.begin();

  Iono.setup();

  // set initial status of digital inputs to unknown
  for (int i = 0; i < 6; i++) {
    values[i] = -1;
    lastvalues[i] = -1;
    times[i] = 0;
  }

  for (int i = 0; i < CLIENTS_MAX; i++) {
    mbapindex[i] = 0;
    pduindex[i] = 0;
  }
}

void loop() {
  debounce();

  BridgeClient client = server.accept();
  if (client) {
    int i = 0;
    for (i = 0; i < CLIENTS_MAX; i++) {
      if (!clients[i]) {
        break;
      }
    }
    if (i >= CLIENTS_MAX) {
      i = 0;
      for (i = 0; i < CLIENTS_MAX; i++) {
        if (millis() - lastrequest[i] >= CLIENT_TIMEOUT) {
          clients[i].stop();
          break;
        }
      }
    }
    if (i < CLIENTS_MAX) {
      clients[i] = client;
      mbapindex[i] = 0;
      pduindex[i] = 0;
      lastrequest[i] = millis();
    } else {
      client.stop();
    }
  }

  for (int i = 0; i < CLIENTS_MAX; i++) {
    if (clients[i]) {
      while (clients[i].available()) {
        byte b = clients[i].read();
        if (b != -1) {
          if (!inputProcessor(i, b)) {
            clients[i].stop();
            break;
          }
        }
      }
    }
  }
}

boolean inputProcessor(int clientIdx, byte b) {
  switch (mbapindex[clientIdx]) {
    case 0:  // transaction identifier
    case 1:
      mbap[clientIdx][mbapindex[clientIdx]++] = b;
      break;
    case 2: // protocol identifier must be 0
    case 3:
      if (b != 0) {
        mbapindex[clientIdx] = 0;
        pduindex[clientIdx] = 0;
        return false;
      }
      mbap[clientIdx][mbapindex[clientIdx]++] = b;
      break;
    case 4: // length, MSB
      mbap[clientIdx][mbapindex[clientIdx]++] = b;
      pdulength[clientIdx] = b << 8;
      break;
    case 5: // length, LSB
      mbap[clientIdx][mbapindex[clientIdx]++] = b;
      pdulength[clientIdx] += b - 1;
      if (pdulength[clientIdx] > 0 && pdulength[clientIdx] <= sizeof(pdu[clientIdx])) {
      } else {
        mbapindex[clientIdx] = 0;
        pduindex[clientIdx] = 0;
        return false;
      }
      break;
    case 6: // unit id
      mbap[clientIdx][mbapindex[clientIdx]] = b;
      mbapindex[clientIdx] = -1;
      break;
    case -1: // reading PDU
      pdu[clientIdx][pduindex[clientIdx]++] = b;
      if (pduindex[clientIdx] == pdulength[clientIdx]) {
        processPdu(clientIdx, mbap[clientIdx], pdu[clientIdx], rpdu[clientIdx]);
        mbapindex[clientIdx] = 0;
        pduindex[clientIdx] = 0;
      }
      break;
    default:
      mbapindex[clientIdx]++;
      break;
  }
  return true;
}

void processPdu(int clientIdx, byte *mbap, byte *pdu, byte *rpdu) {
  unsigned int start, quantity;
  mbap[4] = 0;
  switch (pdu[0]) {
    case 1: // read coils
      // read status of output relays (DO1-DO6), Modbus address 1-6
      if (pdu[1] == 0 && pdu[3] == 0 && pdu[2] > 0 && pdu[4] > 0 && pdu[2] + pdu[4] <= 7) {
        mbap[5] = 4;
        rpdu[0] = rpdu[1] = 1;
        rpdu[2] = 0;
        for (int i = pdu[2] + pdu[4] - 1; i >= pdu[2]; i--) {
          rpdu[2] <<= 1;
          if (Iono.read(indexToDigitalOutput(i)) == HIGH) {
            rpdu[2] += 1;
          }
        }
      } else {
        mbap[5] = 3;
        rpdu[0] = 0x81;
        rpdu[1] = 2; // illegal data address
      }
      break;
    case 2: // read discrete inputs
      // read status of digital inputs (DI1-DI6), Modbus address 101-106 (with de-bouce) and 111-116 (no de-bouce)
      if (pdu[1] == 0 && pdu[3] == 0 && (pdu[2] > 100 && pdu[2] < 107 || pdu[2] > 110 && pdu[2] < 117) && pdu[4] > 0 && pdu[4] < 7) {
        mbap[5] = 4;
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
        mbap[5] = 3;
        rpdu[0] = 0x82;
        rpdu[1] = 2; // illegal data address
      }
      break;
    case 3: // read holding registers
      // read status of analog output (AO1), Modbus address 601
      if (pdu[1] == 2 && pdu[2] == 89 && pdu[3] == 0 && pdu[4] == 1) {
        int v = Iono.read(AO1) * 1000;
        mbap[5] = 5;
        rpdu[0] = 3;
        rpdu[1] = 2;
        rpdu[2] = (byte)(v >> 8);
        rpdu[3] = (byte)(v & 0xff);
      } else {
        mbap[5] = 3;
        rpdu[0] = 0x83;
        rpdu[1] = 2; // illegal data address
      }
      break;
    case 4: // read input registers
      start = (pdu[1] << 8) + pdu[2];
      quantity = (pdu[3] << 8) + pdu[4];
      if (start >= 201 && start <= 204 && start + quantity <= 205) {
        // read status of analog voltage inputs (AV1-AV4), Modbus address 201-204
        mbap[5] = 2 * quantity + 3;
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
        mbap[5] = 2 * quantity + 3;
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
        mbap[5] = 2 * quantity + 3;
        rpdu[0] = 4;
        rpdu[1] = 2 * quantity;
        int v;
        for (int i = 1; i <= quantity; i++) {
          v = counters[i + start - 1002];
          rpdu[i * 2] = (byte)(v >> 8);
          rpdu[1 + i * 2] = (byte)(v & 0xff);
        }
      } else if (start == 64990 && quantity == 4) {
        // read identifier code
        mbap[5] = 8 + 3;
        rpdu[0] = 4;
        rpdu[1] = 8;
        rpdu[2] = 0xCA; // fixed
        rpdu[3] = 0xFE; // fixed
        rpdu[4] = 0xBE; // fixed
        rpdu[5] = 0xAF; // fixed
        rpdu[6] = 0x15; // Iono Arduino (0x10) + Arduino YUN (0x05)
        rpdu[7] = 0x02; // App ID: Modbus TCP
        rpdu[8] = 0x01; // Version High
        rpdu[9] = 0x00; // Version Low
      } else {
        mbap[5] = 3;
        rpdu[0] = 0x84;
        rpdu[1] = 2; // illegal data address
      }
      break;
    case 5: // write single coil
      // command of single output relay (DO1-DO6), Modbus address 1-6
      if (pdu[1] == 0 && pdu[2] > 0 && pdu[2] <= 6) {
        if (pdu[3] == 0 && pdu[4] == 0) {
          Iono.write(indexToDigitalOutput(pdu[2]), LOW);
          mbap[5] = 6;
          for (int i = 0; i < 5; i++) {
            rpdu[i] = pdu[i];
          }
        } else if (pdu[3] == 0xff && pdu[4] == 0) {
          Iono.write(indexToDigitalOutput(pdu[2]), HIGH);
          mbap[5] = 6;
          for (int i = 0; i < 5; i++) {
            rpdu[i] = pdu[i];
          }
        } else {
          mbap[5] = 3;
          rpdu[0] = 0x81;
          rpdu[1] = 3; // illegal data value
        }
      } else {
        mbap[5] = 3;
        rpdu[0] = 0x81;
        rpdu[1] = 2; // illegal data address
      }
      break;
    case 6: // write single register
      // command of analog output (AO1), Modbus address 601, 0-10000 mV
      if (pdu[1] == 2 && pdu[2] == 89) {
        int v = (pdu[3] << 8) + pdu[4];
        if (v >= 0 && v <= 10000) {
          Iono.write(AO1, v / 1000.0);
          mbap[5] = 6;
          for (int i = 0; i < 5; i++) {
            rpdu[i] = pdu[i];
          }
        } else {
          mbap[5] = 3;
          rpdu[0] = 0x86;
          rpdu[1] = 3; // illegal data value
        }
      } else {
        mbap[5] = 3;
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
          mbap[5] = 6;
          for (int i = 0; i < 5; i++) {
            rpdu[i] = pdu[i];
          }
        } else {
          mbap[5] = 3;
          rpdu[0] = 0x8f;
          rpdu[1] = 3; // illegal data value
        }
      } else {
        mbap[5] = 3;
        rpdu[0] = 0x8f;
        rpdu[1] = 2; // illegal data address
      }
      break;
    default: // error
      mbap[5] = 3;
      rpdu[0] = pdu[0] | 0x80;
      rpdu[1] = 1; // illegal function
      break;
  }
  clients[clientIdx].write(mbap, 7);
  clients[clientIdx].write(rpdu, mbap[5] - 1);
  clients[clientIdx].flush();
  lastrequest[clientIdx] = millis();
}

void debounce() {
  int value;
  for (int i = 0; i < 6; i++) {
    value = (Iono.read(indexToDigitalInput(i + 1)) == HIGH) ? 1 : 0;
    if (value != lastvalues[i]) {
      times[i] = millis();
    }
    if ((millis() - times[i]) > DELAY) {
      if (values[i] == -1) {
        values[i] = value;
      } else if (values[i] != value) {
        values[i] = value;
        if (value == 1) {
          counters[i] = (counters[i] < 65535) ? counters[i] + 1 : 0;
        }
      }
    }
    lastvalues[i] = value;
  }
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
