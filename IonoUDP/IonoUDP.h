/*
  IonoUDP.h
*/

#ifndef IonoUDP_h
#define IonoUDP_h

#include <SPI.h>
#include <Ethernet.h>
#include <Iono.h>

#define COMMAND_MAX_SIZE 10

class IonoUDPClass
{
  public:
    IonoUDPClass();
    void begin(const char *id, EthernetUDP Udp, unsigned int port, unsigned long stableTime, float minVariation);
    void process();

  private:
    static char _pinName[][4];

    IPAddress _ipBroadcast;
    float _lastValue[20];
    float _value[20];
    unsigned long _lastTS[20];
    char _progr;
    const char *_id;
    unsigned int _port;
    EthernetUDP _Udp;
    char _command[COMMAND_MAX_SIZE];
    unsigned long _stableTime;
    float _minVariation;
    unsigned long _lastSend;

    void checkState();
    void check(int pin);
    void send(int pin, float val);
    void ftoa(char *sVal, float fVal);
    void checkCommands();
};

extern IonoUDPClass IonoUDP;

#endif