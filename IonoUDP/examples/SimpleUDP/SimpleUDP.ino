#include <SPI.h>
#include <Ethernet.h>
#include <Iono.h>
#include <IonoUDP.h>

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0xD5, 0x8C };

IPAddress ip(192, 168, 1, 243);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

unsigned int port = 7878;

EthernetUDP Udp;

//char macStr[18];

void setup() {
  Ethernet.begin(mac, ip, gateway, subnet);
  Udp.begin(port);

//  for (int i = 0; i < 6; i++) {
//    int j = i * 2;
//    if (i != 0) {
//      j += i - 1;
//      macStr[j++] = ':';
//    }      
//    macStr[j] = (mac[i] >> 4) + 0x30;
//    if (macStr[j] > 0x39) macStr[j] +=7;
//    macStr[j + 1] = (mac[i] & 0x0f) + 0x30;
//    if (macStr[j + 1] > 0x39) macStr[j + 1] +=7;
//  }
//  macStr[17] = '\0';

  IonoUDP.begin("myIono", Udp, port, 50, 0.1);
}

void loop() {
  IonoUDP.process();
  delay(10);
}


