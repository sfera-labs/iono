/*
  IonoModbusTcp.cpp - A Modbus TCP server for Iono Ethernet/MKR

    Copyright (C) 2016-2018 Sfera Labs S.r.l. - All rights reserved.

    For information, see:
    http://www.sferalabs.cc/iono

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  See file LICENSE.txt for further informations on licensing terms.
*/

#ifdef ARDUINO_ARCH_SAMD
#include <FlashAsEEPROM.h>
#include <FlashStorage.h>
#else
#include <EEPROM.h>
#endif

#include <Iono.h>

#ifdef IONO_MKR
  #include <WiFiNINA.h>
#else
  #ifdef ARDUINO_AVR_LEONARDO_ETH
    #include <Ethernet2.h>
  #else
    #include <Ethernet.h>
  #endif
#endif

#include <SPI.h>
#include <avr/pgmspace.h>


#define DELAY  50            // the debounce delay in milliseconds
#define MAX_SSID_PASS_LEN 30

const PROGMEM char CONSOLE_MENU_HEADER[]  = {"=== Sfera Labs - Modbus TCP server configuration menu - v2.1 ==="};
const PROGMEM char CONSOLE_MENU_CURRENT_CONFIG[]  = {"Print current configuration"};
const PROGMEM char CONSOLE_MENU_MAC[]  = {"MAC address (Eth only)"};
const PROGMEM char CONSOLE_MENU_IP[]  = {"IP address"};
const PROGMEM char CONSOLE_MENU_IP_ASSIGNED[]  = {"IP address assigned"};
const PROGMEM char CONSOLE_MENU_MASK[]  = {"Network mask"};
const PROGMEM char CONSOLE_MENU_DNS[]  = {"DNS address"};
const PROGMEM char CONSOLE_MENU_GATEWAY[]  = {"Default gateway"};
const PROGMEM char CONSOLE_MENU_SSID[]  = {"WiFi SSID"};
const PROGMEM char CONSOLE_MENU_PASS[]  = {"WiFi Password"};
const PROGMEM char CONSOLE_MENU_SAVE[]  = {"Save configuration and restart"};
const PROGMEM char CONSOLE_MENU_TYPE[]  = {"Type a menu number [0-8]: "};
const PROGMEM char CONSOLE_TYPE_MAC[]  = {"Type MAC address and press ENTER (NN-NN-NN-NN-NN-NN): "};
const PROGMEM char CONSOLE_TYPE_IP[]  = {"Type IP address (NNN.NNN.NNN.NNN): "};
const PROGMEM char CONSOLE_TYPE_MASK[]  = {"Type Network mask (NNN.NNN.NNN.NNN): "};
const PROGMEM char CONSOLE_TYPE_DNS[]  = {"Type DNS address (NNN.NNN.NNN.NNN): "};
const PROGMEM char CONSOLE_TYPE_GATEWAY[]  = {"Type Default gateway address (NNN.NNN.NNN.NNN): "};
const PROGMEM char CONSOLE_TYPE_SSID[]  = {"Type WiFi SSID: "};
const PROGMEM char CONSOLE_TYPE_PASS[]  = {"Type WiFi Password: "};
const PROGMEM char CONSOLE_TYPE_SAVE[]  = {"Confirm? (Y/N): "};
const PROGMEM char CONSOLE_CURRENT_CONFIG[]  = {"Current network configuration:"};
const PROGMEM char CONSOLE_NEW_CONFIG[]  = {"New network configuration:"};
const PROGMEM char CONSOLE_ERROR[]  = {"Error"};
const PROGMEM char CONSOLE_SAVED[]  = {"Saved"};

#ifdef IONO_MKR
# define EthernetServer WiFiServer
# define EthernetClient WiFiClient

char ssidCurrent[MAX_SSID_PASS_LEN + 1];
char passCurrent[MAX_SSID_PASS_LEN + 1];
char ssidNew[MAX_SSID_PASS_LEN + 1];
char passNew[MAX_SSID_PASS_LEN + 1];
int ledState;
boolean clientAfterReset = false;
unsigned long lastClientTs;
#else
char *ssidCurrent = NULL;
char *passCurrent = NULL;
char *ssidNew = NULL;
char *passNew = NULL;
#endif

EthernetServer server(502); // Modbus TCP server listening on port 502

int consoleState = -1;
boolean serverEnabled;
int analogOutValue = 0; // 0-10000 mV

short values[6];            // current valid state for digital inputs
short lastvalues[6];        // values read in last loop
long times[6];              // last change timestamp
unsigned short counters[6]; // digital inputs counter

int mbapindex = 0; // set to -1 when done reading MBAP header
int pduindex = 0;
int pdulength;

char macCurrent[18];
char ipCurrent[16];
char netmaskCurrent[16];
char gatewayCurrent[16];
char dnsCurrent[16];
char macNew[18];
char ipNew[16];
char netmaskNew[16];
char gatewayNew[16];
char dnsNew[16];
byte mbap[7];
byte pdu[16];
byte rpdu[16];

bool serialStarted = false;

void setup() {
  // serial console menu
  Serial.begin(9600);

  // retrieve network settings from EEPROM and initialize server
  if (getNetConfigAndSet()) {
    server.begin();
    serverEnabled = true;
  } else {
    serverEnabled = false;
  }

  // set initial status of digital inputs to unknown
  for (int i = 0; i < sizeof(values) / sizeof(int); i++) {
    values[i] = -1;
    lastvalues[i] = -1;
    times[i] = 0;
  }

#ifdef IONO_MKR
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
#endif
}

void loop() {
  // serial console processor
  serialConsole();

  // digital inputs debouncer and counter
  debounce();

  // Modbus TCP server
  if (serverEnabled) {
#ifdef IONO_MKR
    ledState = HIGH;
    if (consoleState == -1) {
      if (WiFi.status() == WL_CONNECTED) {
        ledState = LOW;
        if (clientAfterReset && millis() - lastClientTs >= 10000) {
          WiFi.end();
        }
      } else {
        WiFi.end();
        WiFi.begin(ssidCurrent, passCurrent);
        server.begin();
        clientAfterReset = false;
      }
    }
    digitalWrite(LED_BUILTIN, ledState);
#endif

    EthernetClient client = server.available();

    if (client) {
      while (client.available()) {
        byte b = client.read();
        if (b != -1) {
          if (!inputProcessor(client, b)) {
            client.stop();
            break;
          }
        }
      }
#ifdef IONO_MKR
      lastClientTs = millis();
      clientAfterReset = true;
#endif
    }
  }
}

boolean inputProcessor(EthernetClient client, byte b) {
  switch (mbapindex) {
    case 0:  // transaction identifier
    case 1:
      mbap[mbapindex++] = b;
      break;
    case 2: // protocol identifier must be 0
    case 3:
      if (b != 0) {
        mbapindex = 0;
        pduindex = 0;
        return false;
      }
      mbap[mbapindex++] = b;
      break;
    case 4: // length, MSB
      mbap[mbapindex++] = b;
      pdulength = b << 8;
      break;
    case 5: // length, LSB
      mbap[mbapindex++] = b;
      pdulength += b - 1;
      if (pdulength > 0 && pdulength <= sizeof(pdu)) {
      } else {
        mbapindex = 0;
        pduindex = 0;
        return false;
      }
      break;
    case 6: // unit id
      mbap[mbapindex] = b;
      mbapindex = -1;
      break;
    case -1: // reading PDU
      pdu[pduindex++] = b;
      if (pduindex == pdulength) {
        processPdu(client, mbap, pdu);
        mbapindex = 0;
        pduindex = 0;
      }
      break;
    default:
      mbapindex++;
      break;
  }
  return true;
}

void processPdu(EthernetClient client, byte *mbap, byte *pdu) {
  int start, quantity;
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
        mbap[5] = 5;
        rpdu[0] = 3;
        rpdu[1] = 2;
        rpdu[2] = (byte)(analogOutValue >> 8);
        rpdu[3] = (byte)(analogOutValue & 0xff);
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
        rpdu[6] = 0x10; // Iono Arduino (0x10)
        rpdu[7] = 0x02; // App ID: Modbus TCP
        rpdu[8] = 0x02; // Version High
        rpdu[9] = 0x01; // Version Low
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
          analogOutValue = v;
          Iono.write(AO1, analogOutValue / 1000.0);
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
  client.write(mbap, 7);
  client.write(rpdu, mbap[5] - 1);
  client.flush();
}

void serialConsole() {
  if (!Serial) {
    return;
  }

  if (!serialStarted) {
    serialStarted = true;
    consoleState = 0;
    printConsoleMenu();
  }

  if (Serial.available() > 0) {
    int c = Serial.read();
    switch (consoleState) {
      case 0: // waiting for menu selection number
        switch (c) {
          case '0':
            Serial.println((char)c);
            Serial.println();
            printlnProgMemString(CONSOLE_CURRENT_CONFIG);
            printConfiguration(macCurrent, ipCurrent, netmaskCurrent, dnsCurrent, gatewayCurrent, ssidCurrent, passCurrent, true);
            printConsoleMenu();
            break;
          case '1':
            consoleState = 1;
            macNew[0] = 0;
            Serial.println((char)c);
            Serial.println();
            printProgMemString(CONSOLE_TYPE_MAC);
            break;
          case '2':
            consoleState = 2;
            ipNew[0] = 0;
            Serial.println((char)c);
            Serial.println();
            printProgMemString(CONSOLE_TYPE_IP);
            break;
          case '3':
            consoleState = 3;
            netmaskNew[0] = 0;
            Serial.println((char)c);
            Serial.println();
            printProgMemString(CONSOLE_TYPE_MASK);
            break;
          case '4':
            consoleState = 4;
            dnsNew[0] = 0;
            Serial.println((char)c);
            Serial.println();
            printProgMemString(CONSOLE_TYPE_DNS);
            break;
          case '5':
            consoleState = 5;
            gatewayNew[0] = 0;
            Serial.println((char)c);
            Serial.println();
            printProgMemString(CONSOLE_TYPE_GATEWAY);
            break;
          case '6':
            consoleState = 6;
            ssidNew[0] = 0;
            Serial.println((char)c);
            Serial.println();
            printProgMemString(CONSOLE_TYPE_SSID);
            break;
          case '7':
            consoleState = 7;
            passNew[0] = 0;
            Serial.println((char)c);
            Serial.println();
            printProgMemString(CONSOLE_TYPE_PASS);
            break;
          case '8':
            consoleState = 8;
            Serial.println((char)c);
            Serial.println();
            printlnProgMemString(CONSOLE_NEW_CONFIG);
            printConfiguration(
              (macNew[0] == 0) ? macCurrent : macNew,
              (ipNew[0] == 0) ? ipCurrent : ipNew,
              (netmaskNew[0] == 0) ? netmaskCurrent : netmaskNew,
              (dnsNew[0] == 0) ? dnsCurrent : dnsNew,
              (gatewayNew[0] == 0) ? gatewayCurrent : gatewayNew,
              (ssidNew[0] == 0) ? ssidCurrent : ssidNew,
              (passNew[0] == 0) ? passCurrent : passNew,
              false
            );
            printProgMemString(CONSOLE_TYPE_SAVE);
            break;
        }
        break;
      case 1: // MAC address
        if (stringEdit(macNew, c, 17, true, true, '-')) {
          byte maca[6];
          Serial.println();
          if (!parseMacAddress(macNew, maca)) {
            printlnProgMemString(CONSOLE_ERROR);
            macNew[0] = 0;
          }
          consoleState = 0;
          printConsoleMenu();
        }
        break;
      case 2: // IP address
        if (stringEdit(ipNew, c, 15, true, false, '.')) {
          byte ipa[6];
          Serial.println();
          if (!parseIpAddress(ipNew, ipa)) {
            printlnProgMemString(CONSOLE_ERROR);
            ipNew[0] = 0;
          }
          consoleState = 0;
          printConsoleMenu();
        }
        break;
      case 3: // subnet mask
        if (stringEdit(netmaskNew, c, 15, true, false, '.')) {
          byte netmaska[6];
          Serial.println();
          if (!parseIpAddress(netmaskNew, netmaska)) {
            printlnProgMemString(CONSOLE_ERROR);
            netmaskNew[0] = 0;
          }
          consoleState = 0;
          printConsoleMenu();
        }
        break;
      case 4: // DNS
        if (stringEdit(dnsNew, c, 15, true, false, '.')) {
          byte dnsa[6];
          Serial.println();
          if (!parseIpAddress(dnsNew, dnsa)) {
            printlnProgMemString(CONSOLE_ERROR);
            dnsNew[0] = 0;
          }
          consoleState = 0;
          printConsoleMenu();
        }
        break;
      case 5: // gateway
        if (stringEdit(gatewayNew, c, 15, true, false, '.')) {
          byte ipa[6];
          Serial.println();
          if (!parseIpAddress(gatewayNew, ipa)) {
            printlnProgMemString(CONSOLE_ERROR);
            gatewayNew[0] = 0;
          }
          consoleState = 0;
          printConsoleMenu();
        }
        break;
      case 6: // ssid
        if (stringEdit(ssidNew, c, MAX_SSID_PASS_LEN, false, false, 0)) {
          Serial.println();
          consoleState = 0;
          printConsoleMenu();
        }
        break;
      case 7: // pass
        if (stringEdit(passNew, c, MAX_SSID_PASS_LEN, false, false, 0)) {
          Serial.println();
          consoleState = 0;
          printConsoleMenu();
        }
        break;
      case 8: // confirm to save
        switch (c) {
          case 'Y':
          case 'y':
            consoleState = 0;
            Serial.println('Y');
            if (saveNetConfigAndRestart()) {
              printlnProgMemString(CONSOLE_SAVED);
            } else {
              printlnProgMemString(CONSOLE_ERROR);
            }
            printConsoleMenu();
            break;
          case 'N':
          case 'n':
            consoleState = 0;
            Serial.println('N');
            Serial.println();
            printConsoleMenu();
            break;
        }
        break;
      default:
        break;
    }
  }
}

void debounce() {
  int value;
  for (int i = 0; i < sizeof(values) / sizeof(int); i++) {
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

boolean saveNetConfigAndRestart() {
  /*
  byte maca[6];
  byte ipa[4];
  byte netmaska[4];
  byte dnsa[4];
  byte gatewaya[4];
  */
  if (macNew[0] == 0) {
    strcpy(macNew, macCurrent);
  }
  if (ipNew[0] == 0) {
    strcpy(ipNew, ipCurrent);
  }
  if (netmaskNew[0] == 0) {
    strcpy(netmaskNew, netmaskCurrent);
  }
  if (dnsNew[0] == 0) {
    strcpy(dnsNew, dnsCurrent);
  }
  if (gatewayNew[0] == 0) {
    strcpy(gatewayNew, gatewayCurrent);
  }
#ifdef IONO_MKR
  if (ssidNew[0] == 0) {
    strcpy(ssidNew, ssidCurrent);
  }
  if (passNew[0] == 0) {
    strcpy(passNew, passCurrent);
  }
#endif
  if (writeEepromConfig(macNew, ipNew, netmaskNew, dnsNew, gatewayNew, ssidNew, passNew)) {
    softReset();
    return true;
  } else {
    return false;
  }
}

boolean getNetConfigAndSet() {
  byte maca[6];
  byte ipa[4];
  byte netmaska[4];
  byte dnsa[4];
  byte gatewaya[4];

  if (readEepromConfig(maca, ipa, netmaska, dnsa, gatewaya, ssidCurrent, passCurrent)) {
    printMacAddress(macCurrent, maca);
    printIpAddress(ipCurrent, ipa);
    printIpAddress(netmaskCurrent, netmaska);
    printIpAddress(dnsCurrent, dnsa);
    printIpAddress(gatewayCurrent, gatewaya);
    IPAddress ip(ipa[0], ipa[1], ipa[2], ipa[3]);
    IPAddress dns(dnsa[0], dnsa[1], dnsa[2], dnsa[3]);
    IPAddress subnet(netmaska[0], netmaska[1], netmaska[2], netmaska[3]);
    IPAddress gateway(gatewaya[0], gatewaya[1], gatewaya[2], gatewaya[3]);
#ifdef IONO_MKR
    WiFi.config(ip, dns, gateway, subnet);
    WiFi.begin(ssidCurrent, passCurrent);
#else
    Ethernet.begin(maca, ip, dns, gateway, subnet);
#endif
    return true;
  } else {
    return false;
  }
}

boolean writeEepromConfig(char *mac, char *ip, char *netmask, char *dns, char *gateway, char *ssid, char *pass) {
  byte maca[6];
  byte ipa[4];
  byte netmaska[4];
  byte dnsa[4];
  byte gatewaya[4];
  if ((mac[0] != 0 || (ssid[0] != 0 && pass[0] != 0)) && ip[0] != 0 && netmask[0] != 0 && dns[0] != 0 && gateway[0] != 0) {
    if (parseMacAddress(mac, maca) && parseIpAddress(ip, ipa) && parseIpAddress(netmask, netmaska) && parseIpAddress(dns, dnsa) && parseIpAddress(gateway, gatewaya)) {
      byte checksum = 7;
      int a = 0;
      for (; a < 4; a++) {
        EEPROM.write(a, ipa[a]);
        checksum ^= ipa[a];
      }
      for (; a < 8; a++) {
        EEPROM.write(a, dnsa[a - 4]);
        checksum ^= dnsa[a - 4];
      }
      for (; a < 12; a++) {
        EEPROM.write(a, gatewaya[a - 8]);
        checksum ^= gatewaya[a - 8];
      }
      for (; a < 16; a++) {
        EEPROM.write(a, netmaska[a - 12]);
        checksum ^= netmaska[a - 12];
      }
#ifndef IONO_MKR
      for (; a < 22; a++) {
        EEPROM.write(a, maca[a - 16]);
        checksum ^= maca[a - 16];
      }
#else
      for (; a < 16 + MAX_SSID_PASS_LEN + 1; a++) {
        EEPROM.write(a, ssid[a - 16]);
        checksum ^= ssid[a - 16];
        if (ssid[a - 16] == 0) {
          a++;
          break;
        }
      }
      int ssidLen = a - 16;

      for (; a < 16 + ssidLen + MAX_SSID_PASS_LEN + 1; a++) {
        EEPROM.write(a, pass[a - 16 - ssidLen]);
        checksum ^= pass[a - 16 - ssidLen];
        if (pass[a - 16 - ssidLen] == 0) {
          a++;
          break;
        }
      }
#endif
      EEPROM.write(a, checksum);

#ifdef ARDUINO_ARCH_SAMD
      EEPROM.commit();
#endif

      return true;
    }
  }
  return false;
}

boolean readEepromConfig(byte *maca, byte *ipa, byte *netmaska, byte *dnsa, byte *gatewaya, char *ssid, char *pass) {
#ifdef ARDUINO_ARCH_SAMD
  if (!EEPROM.isValid()) {
    return false;
  }
#endif

  byte checksum = 7;
  int a = 0;
  for (; a < 4; a++) {
    ipa[a] = EEPROM.read(a);
    checksum ^= ipa[a];
  }
  for (; a < 8; a++) {
    dnsa[a - 4] = EEPROM.read(a);
    checksum ^= dnsa[a - 4];
  }
  for (; a < 12; a++) {
    gatewaya[a - 8] = EEPROM.read(a);
    checksum ^= gatewaya[a - 8];
  }
  for (; a < 16; a++) {
    netmaska[a - 12] = EEPROM.read(a);
    checksum ^= netmaska[a - 12];
  }
#ifndef IONO_MKR
  for (; a < 22; a++) {
    maca[a - 16] = EEPROM.read(a);
    checksum ^= maca[a - 16];
  }
#else
  byte acam[6];
  WiFi.macAddress(acam);
  for (int i = 0; i < 6; i++) {
    maca[i] = acam[5 - i];
  }

  for (; a < 16 + MAX_SSID_PASS_LEN + 1; a++) {
    ssid[a - 16] = EEPROM.read(a);
    checksum ^= ssid[a - 16];
    if (ssid[a - 16] == 0) {
      a++;
      break;
    }
  }
  int ssidLen = a - 16;
  ssid[ssidLen - 1] = 0;

  for (; a < 16 + ssidLen + MAX_SSID_PASS_LEN + 1; a++) {
    pass[a - 16 - ssidLen] = EEPROM.read(a);
    checksum ^= pass[a - 16 - ssidLen];
    if (pass[a - 16 - ssidLen] == 0) {
      a++;
      break;
    }
  }
  int passLen = a - 16 - ssidLen;
  pass[passLen - 1] = 0;
#endif

  return (EEPROM.read(a) == checksum);
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
  Serial.println();
}

void printProgMemString(const char* s) {
  int len = strlen_P(s);
  for (int k = 0; k < len; k++) {
    Serial.print((char)pgm_read_byte_near(s + k));
  }
}

void printConsoleMenu() {
  Serial.println();
  printlnProgMemString(CONSOLE_MENU_HEADER);
  for (int i = 0; i <= 8; i++) {
    Serial.print(i);
    Serial.print(". ");
    switch (i) {
      case 0:
        printlnProgMemString(CONSOLE_MENU_CURRENT_CONFIG);
        break;
      case 1:
        printlnProgMemString(CONSOLE_MENU_MAC);
        break;
      case 2:
        printlnProgMemString(CONSOLE_MENU_IP);
        break;
      case 3:
        printlnProgMemString(CONSOLE_MENU_MASK);
        break;
      case 4:
        printlnProgMemString(CONSOLE_MENU_DNS);
        break;
      case 5:
        printlnProgMemString(CONSOLE_MENU_GATEWAY);
        break;
      case 6:
        printlnProgMemString(CONSOLE_MENU_SSID);
        break;
      case 7:
        printlnProgMemString(CONSOLE_MENU_PASS);
        break;
      case 8:
        printlnProgMemString(CONSOLE_MENU_SAVE);
        break;
    }
  }
  printProgMemString(CONSOLE_MENU_TYPE);
}

void printConfiguration(char *mac, char *ip, char *netmask, char *dns, char *gateway, char *ssid, char *pass, bool printAssignedIp) {
  char s[] = ": ";
  printProgMemString(CONSOLE_MENU_MAC);
  Serial.print(s);
  Serial.println(mac);
  printProgMemString(CONSOLE_MENU_IP);
  Serial.print(s);
  Serial.println(ip);
  if (printAssignedIp) {
    printProgMemString(CONSOLE_MENU_IP_ASSIGNED);
    Serial.print(s);
#ifdef IONO_MKR
    Serial.println(WiFi.localIP());
#else
    Serial.println(Ethernet.localIP());
#endif
  }
  printProgMemString(CONSOLE_MENU_MASK);
  Serial.print(s);
  Serial.println(netmask);
  printProgMemString(CONSOLE_MENU_DNS);
  Serial.print(s);
  Serial.println(dns);
  printProgMemString(CONSOLE_MENU_GATEWAY);
  Serial.print(s);
  Serial.println(gateway);
#ifdef IONO_MKR
  printProgMemString(CONSOLE_MENU_SSID);
  Serial.print(s);
  Serial.println(ssid);
  printProgMemString(CONSOLE_MENU_PASS);
  Serial.print(s);
  Serial.println(pass);
#endif
}

boolean stringEdit(char *s, int c, int size, bool filter, bool hex, char sep) {
  int i;
  switch (c) {
    case 8: case 127: // backspace
      i = strlen(s);
      if (i > 0) {
        Serial.print('\b');
        Serial.print(' ');
        Serial.print('\b');
        s[i - 1] = 0;
      }
      break;
    case 10: // newline
    case 13: // enter
      if (strlen(s) > 0) {
        return true;
      }
      break;
    default:
      if (strlen(s) < size) {
        if (filter && hex && c >= 'a' && c <= 'f') {
          c -= 32;
        }
        if (!filter || c >= '0' && c <= '9' || (hex && (c >= 'A' && c <= 'F'))) {
          Serial.print((char)c);
          strcat_c(s, c);
        } else {
          Serial.print(sep);
          strcat_c(s, sep);
        }
      }
      break;
  }
  return false;
}

boolean parseIpAddress(char *s, byte a[]) {
  if (s[0] == 0) {
    return true;
  }
  for (int i = 0; i < 4; i++) {
    if (s[0] == 0) {
      return false;
    }
    long v = strtol(s, &s, 10);
    if (v < 0 || v > 255) {
      return false;
    }
    a[i] = v;
    s++;
  }
  return true;
}

boolean parseMacAddress(char *s, byte a[]) {
  if (s[0] == 0) {
    return true;
  }
  for (int i = 0; i < 6; i++) {
    if (s[0] == 0) {
      return false;
    }
    long v = strtol(s, &s, 16);
    if (v < 0 || v > 255) {
      return false;
    }
    a[i] = v;
    s++;
  }
  return true;
}

void printIpAddress(char *s, byte ip[]) {
  sprintf(s, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

void printMacAddress(char *s, byte mac[]) {
  sprintf(s, "%02X.%02X.%02X.%02X.%02X.%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
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
