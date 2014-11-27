/*
  IonoWeb.h
*/

#ifndef IonoWeb_h
#define IonoWeb_h

#include <SPI.h>
#include <Ethernet.h>
#include <Iono.h>
#define WEBDUINO_NO_IMPLEMENTATION true
#include "WebServer.h"

class IonoWebClass
{
  public:
    static void begin(int port);
    static void processRequest();
    static void setDefaultCommand(WebServer::Command *cmd);
    static void subscribeDigital(int pin, unsigned long stableTime, char *host, int port, char *command);
    static void subscribeAnalog(int pin, unsigned long stableTime, float minVariation, char *host, int port, char *command);
    static void subscribeCommand(WebServer &webServer, WebServer::ConnectionType type, char* urlTail, bool tailComplete);
    static void setCommand(WebServer &webServer, WebServer::ConnectionType type, char* urlTail, bool tailComplete);
    static void jsonStateCommand(WebServer &webServer, WebServer::ConnectionType type, char* urlTail, bool tailComplete);

  private:
    static WebServer _webServer;

    struct HttpURL
    {
      const char *host;
      int port;
      const char *command;
    };
    static HttpURL _url1;
    static HttpURL _url2;
    static HttpURL _url3;
    static HttpURL _url4;
    static HttpURL _url5;
    static HttpURL _url6;

    static void callDigitalURL(int pin, float value);
    static void callAnalogURL(int pin, float value);
    static void callURL(HttpURL *url, const char *pin, const char *value);
    static void ftoa(char *sVal, float fVal);
};

extern IonoWebClass IonoWeb;

#endif