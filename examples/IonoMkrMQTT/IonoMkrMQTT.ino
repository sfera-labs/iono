/*
  IonoMkrMQTT.cpp

    Copyright (C) 2020-2023 Sfera Labs S.r.l. - All rights reserved.

    For information, see:
    https://www.sferalabs.cc/

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any nlater version.
  See file LICENSE.txt for further informations on licesing terms.
*/

#include <Iono.h>
#include "Watchdog.h"
#include "SerialConfig.h"

#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>

#define DEBOUNCE_MS 25
#define VALIN 0
#define VALCOUNT 1
#define VALOUT 2
#define VALAO1 3
#define DISCONNECTED 6

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

uint8_t in1;
uint8_t in2;
uint8_t in3;
uint8_t in4;
float lastSentIn[] = {-1, -1, -1, -1, -1, -1};
uint16_t lastSentCount[] = {(uint16_t) -1, (uint16_t) -1, (uint16_t) -1, (uint16_t) -1, (uint16_t) -1, (uint16_t) -1};
uint16_t valCount[] = {0, 0, 0, 0, 0, 0};
uint8_t lastSentOut[] = {(uint8_t) -1, (uint8_t) -1, (uint8_t) -1, (uint8_t) -1};
float lastSentAO1 = -1;
unsigned long previousMillis = 0;
String baseTopic = "";
const int suffTopicValLength = 7;
const int suffTopicCountLength = 9;
char labelsIn[6][sizeof(baseTopic)+suffTopicValLength];
char labelsOut[4][sizeof(baseTopic)+suffTopicValLength];
char labelsCount[6][sizeof(baseTopic)+suffTopicCountLength];
unsigned long lastUpdateSendTs;
unsigned long lastFullStateSendTs;
bool needToSend = false;
bool duplicate = false;
bool retain;
uint8_t qos;
String topicAO1;
bool watchdog;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  SerialConfig.setup();
  while (!SerialConfig.isConfigured || SerialConfig.isAvailable) {
    SerialConfig.process();
  }

  baseTopic = SerialConfig.rootTopic[0] == '\0' ? "/" : SerialConfig.rootTopic;
  topicAO1 = String(baseTopic + "ao1" + "/val" + '\0');

  watchdog = SerialConfig.watchdog == 'T' ? true : false;

  initialize();

  lastUpdateSendTs = lastFullStateSendTs = millis();
  digitalWrite(LED_BUILTIN, LOW);

  // set the message receive callback
  mqttClient.onMessage(messageReceived);

  mqttClient.setId(SerialConfig.clientId);
  mqttClient.setUsernamePassword(SerialConfig.username, SerialConfig.password);
  mqttClient.setKeepAliveInterval(atoi(SerialConfig.keepAlive) * 1000);
  retain = SerialConfig.retain == 'T' ? true : false;
  switch (SerialConfig.qos) {
    case '0':
      qos = 0;
      break;
    case '1':
      qos = 1;
      break;
    case '2':
      qos = 2;
  }

  setWillMessage();

  Serial.println("Hello!");
}

// set a will message, used by the broker when the connection dies unexpectantly
void setWillMessage() {
  String willPayload = SerialConfig.willPayload;
  String willTopic = SerialConfig.willTopic;
  if (willTopic.length() > 0 && willPayload.length() > 0) {
    mqttClient.beginWill(String(baseTopic + willTopic + '\0'), willPayload.length(), retain, qos);
    mqttClient.print(willPayload);
    mqttClient.endWill();
  }
}

void connectToWifi() {
  static unsigned long firstConnAttempt = millis();
  if (WiFi.begin(SerialConfig.ssid, SerialConfig.netpass) != WL_CONNECTED) {
    Serial.print(".");
    if (millis() - firstConnAttempt > 60000) {
      WiFi.end();
      Serial.print(" ");
      firstConnAttempt = millis();
    }
  } else {
    Serial.print("\nIP Address: ");
    Serial.println(WiFi.localIP());
  }
}

void connectToBroker() {
  static unsigned long firstConnAttempt = millis();
  Serial.println("Connecting to MQTT broker...");
  mqttClient.setConnectionTimeout(5000);
  if (!mqttClient.connect(SerialConfig.brokerAddr, atoi(SerialConfig.numPort))) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    if (millis() - firstConnAttempt > 60000) {
      WiFi.end();
      firstConnAttempt = millis();
    }
  } else {
    // the MQTT broker doesn't memorize session info about this board,
    // so every time there's a reconnection all subscriptions must be re-done
    Serial.println("Subscribing...");
    subscribeToAll();
    Serial.println("Ready");
  }
}

void subscribeToAll() {
  mqttClient.subscribe(labelsOut[0], qos);
  mqttClient.subscribe(labelsOut[1], qos);
  mqttClient.subscribe(labelsOut[2], qos);
  mqttClient.subscribe(labelsOut[3], qos);
  mqttClient.subscribe(topicAO1, qos);
}

void loop() {
  Iono.process();

  if (watchdog) {
    Watchdog.clear();
  }

  if (WiFi.status() != WL_CONNECTED) {
    connectToWifi();

  } else if (!mqttClient.connected()) {
    connectToBroker();

  } else {
    unsigned long now = millis();

    if (now - previousMillis >= 100) {
      // poll() calls ping() every keepaliveinterval (default 60 secs) to allow the library to send MQTT keep alives.
      // It also calls messageReceived if there are bytes to read. poll() takes about 1 millisec
      mqttClient.poll();
      previousMillis = now;
    }

    // periodically resend all input statuses
    if (now - lastFullStateSendTs >= 15 * 60000 || now - lastUpdateSendTs >= 7 * 60000) {
      sendState(true);
      lastFullStateSendTs = now;
    } else if (needToSend) {
      sendState(false);
    }
  }

  if (SerialConfig.isAvailable) {
    SerialConfig.process();
  }
}

void send(int opt, int val, int i) {
  if (opt == VALIN) {
    mqttClient.beginMessage(labelsIn[i], retain, qos, duplicate);
  }
  else if (opt == VALCOUNT) {
    mqttClient.beginMessage(labelsCount[i], retain, qos, duplicate);
  }
  else if (opt == VALOUT) {
    mqttClient.beginMessage(labelsOut[i], retain, qos, duplicate);
  }
  else {
    mqttClient.beginMessage(topicAO1, retain, qos, duplicate);
  }
  mqttClient.print(val);
  mqttClient.endMessage();
}

bool sendState(bool sendAll) {
  float valIn[6];
  uint8_t valOut[4];
  float valAO1;
  bool sent = false;
  int inputToSend;
  char mode;

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

  // send input statuses
  for (int i = 0; i < 6; i++) {
    if ((SerialConfig.modes[i] != '-') && (sendAll || lastSentIn[i] != valIn[i])) {
      mode = SerialConfig.modes[i];
      // if input is voltage convert into millivolt, if input is amperage convert into microampere
      inputToSend = (mode == 'V' || mode == 'I') ? valIn[i] * 1000 : valIn[i];
      send(VALIN, inputToSend, i);
      sent = true;
    }
  }

  // send digital input counters
  for (int i = 0; i < 6; i++) {
    if (SerialConfig.modes[i] == 'D') {
      if (sendAll || lastSentCount[i] != valCount[i]) {
        send(VALCOUNT, valCount[i], i);
        sent = true;
      }
    }
  }

  // send digital output statuses
  for (int i = 0; i < 4; i++) {
    if (sendAll || lastSentOut[i] != valOut[i]) {
      send(VALOUT, valOut[i], i);
      sent = true;
    }
  }

  // send analog output status
  if (sendAll || lastSentAO1 != valAO1) {
    send(VALAO1, (int) (valAO1 * 1000), 0);
    sent = true;
  }

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

  if (sent) {
    lastUpdateSendTs = millis();
  }

  needToSend = false;

  digitalWrite(LED_BUILTIN, LOW);
}

//mqtt receive callback function
void messageReceived(int messageSize) {
  String topic = mqttClient.messageTopic();
  String payload = mqttClient.readString();

  if (topic.equals(labelsOut[0])) {
    Iono.write(DO1, payload.equals("0") ? LOW : HIGH);
    lastSentOut[0] = -1;
  }
  else if (topic.equals(labelsOut[1])) {
    Iono.write(DO2, payload.equals("0") ? LOW : HIGH);
    lastSentOut[1] = -1;
  }
  else if (topic.equals(labelsOut[2])) {
    Iono.write(DO3, payload.equals("0") ? LOW : HIGH);
    lastSentOut[2] = -1;
  }
  else if (topic.equals(labelsOut[3])) {
    Iono.write(DO4, payload.equals("0") ? LOW : HIGH);
    lastSentOut[3] = -1;
  }
  else if (topic.equals(topicAO1)) {
    // convert voltage from millivolt to volt
    Iono.write(AO1, payload.toDouble() / 1000.0);
    lastSentAO1 = -1;
  }
}

// a value has changed and need to be sent, increment digital input counters
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
      // increment digital input counters
      valCount[idx]++;
    }
  }

  needToSend = true;
}

void initialize() {
  if (watchdog) {
    Watchdog.setup();
  }

  Iono.setup();

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

  String label;
  for (int i = 0; i < 6; i++) {
    switch (SerialConfig.modes[i]) {
      case 'D':
        label = String(baseTopic + "di" + (i+1) + "/val" + '\0');
        label.toCharArray(labelsIn[i], label.length());
        label = String(baseTopic + "di" + (i+1) + "/count" + '\0');
        label.toCharArray(labelsCount[i], label.length());
        break;
      case 'V':
        label = String(baseTopic + "av" + (i+1) + "/val" + '\0');
        label.toCharArray(labelsIn[i], label.length());
        break;
      case 'I':
        label = String(baseTopic + "ai" + (i+1) + "/val" + '\0');
        label.toCharArray(labelsIn[i], label.length());
        break;
      default:
        break;
    }
  }

  for (int i = 0; i < 4; i++) {
    label = String(baseTopic + "do" + (i+1) + "/val" + '\0');
    label.toCharArray(labelsOut[i], label.length());
  }
}

// predispose inputs respecting serial configuration
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
