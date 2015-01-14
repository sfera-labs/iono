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

void setup() {
  Ethernet.begin(mac, ip, gateway, subnet);
  Udp.begin(port);
  IonoUDP.begin("myIono", Udp, port, 50, 0.1);
}

void loop() {
  IonoUDP.process();
  delay(10);
}


