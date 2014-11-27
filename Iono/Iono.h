/*
  Iono.h
*/

#ifndef Iono_h
#define Iono_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define DO1 0
#define DO2 1
#define DO3 2
#define DO4 3
#define DO5 4
#define DO6 5

#define DI1 6
#define AV1 7
#define AI1 8

#define DI2 9
#define AV2 10
#define AI2 11

#define DI3 12
#define AV3 13
#define AI3 14

#define DI4 15
#define AV4 16
#define AI4 17

#define DI5 18
#define DI6 19
#define AO1 20

class IonoClass
{
  public:
    typedef void Callback(int pin, float value);
    IonoClass();
    float read(int pin);
    void write(int pin, float value);
    void flip(int pin);
    void subscribeDigital(int pin, unsigned long stableTime, Callback *callback);
    void subscribeAnalog(int pin, unsigned long stableTime, float minVariation, Callback *callback);
    void process();

  private:
    int _pinMap[21];

    struct CallbackMap
    {
      int pin;
      unsigned long stableTime;
      float minVariation;
      Callback *callback;
      float lastValue;
      float value;
      unsigned long lastTS;
    };
    CallbackMap _i1;
    CallbackMap _i2;
    CallbackMap _i3;
    CallbackMap _i4;
    CallbackMap _i5;
    CallbackMap _i6;
    CallbackMap _o1;
    CallbackMap _o2;
    CallbackMap _o3;
    CallbackMap _o4;
    CallbackMap _o5;
    CallbackMap _o6;

    void check(CallbackMap *input);
};

extern IonoClass Iono;

#endif