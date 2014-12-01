#include <SPI.h>
#include <Ethernet.h>
#include <Iono.h>
#include <WebServer.h>
#include <IonoWeb.h>

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0xD5, 0x8C };

IPAddress ip(192, 168, 1, 243);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

void setup() {
  Ethernet.begin(mac, ip, gateway, subnet);
  IonoWeb.begin(80);
  
  /* 
  / Every time DI1 changes value
  / and is stable for 500ms
  / execute an HTTP GET request to
  / "192.168.1.242:8080/foo?$pin=$val"
  / where "$pin" is substituted by the 
  / name of the pin (i.e. "DI1") and
  / "$val" by the current value of 
  / the pin (i.e. 1 or 0)
  / Request example:
  / http://192.168.1.242:8080/foo?DI1=1
  */
  IonoWeb.subscribeDigital(DI1, 500, "192.168.1.242", 8080, "/foo?$pin=$val");
  
  /*
  / Every time the voltage on AV2
  / changes of a value >= 1V
  / execute an HTTP GET request to
  / "192.168.1.242:8080/bar?$pin=$val"
  / where "$pin" is substituted by the 
  / name of the pin (i.e. "AV2") and
  / "$val" by the current value of 
  / the pin (e.g. "5.40")
  / Request example:
  / http://192.168.1.242:8080/bar?AV2=5.40
  */
  IonoWeb.subscribeAnalog(AV2, 0, 1, "192.168.1.242", 8080, "/bar?$pin=$val");
}

void loop() {
  // Check all the inputs
  Iono.process();
}

