# Iono - Arduino lybraries
## Iono
This library provides simple functions to control iono's realys and outputs and to monitor its inputs.

To import the library in your skeych use this code:
```
#include <Iono.h>
```

The library includes the following integer constants, that corresponds to iono's pins (inputs/outputs):
```
DO1
DO2
DO3
DO4
DO5
DO6

DI1
AV1
AI1

DI2
AV2
AI2

DI3
AV3
AI3

DI4
AV4
AI4

DI5
DI6
AO1
```

and the following methods:

### Iono.read(int pin)
This method returns the value read from the specified pin.
If the specified pin is digital (i.e. `D01`...`D06`, `DI1`...`DI6`) the returnd value can be `HIGH` or `LOW`.
If the specified pin is an analog voltage (i.e. `AV1`...`AV4`) the returned value is a float ranging from 0.0 to 10.0.
If the specified pin is an analog current (i.e. `AI1`...`AI4`) the returned value is a float ranging from 0.0 to 20.0.

### Iono.write(int pin, float value)
This method writes the passed value to the specified pin.
If the specified pin is a digital output (i.e. `D01`...`D06`) the accepted values are `HIGH` or `LOW`.
If the specified pin is `AO1` the accepted values ranges from 0.0 to 10.0

### Iono.flip(int pin)
This method switches the state of the specified digital output pin (i.e. `D01`...`D06`).

### Iono.subscribeDigital(int pin, unsigned long stableTime, Callback *callback);
This method can be used to attach a callback method to the change of state of a digital input.

The `callback` parameter must point to a void function accepting two parameters: an integer and a float; for instance:
```
void myCallback(int pin, float value)
```
This function will be called every time the specified pin changes state and mantains such state at least for a time equal to the `stableTime` parameter, in milliseconds.
The parameters passed to the callback function will correspond to the monitored pin and the value that triggered the call.

### Iono.subscribeAnalog(int pin, unsigned long stableTime, float minVariation, Callback *callback)
This method can be used to attach a callback method to the change of state of an analog input.

The `callback` parameter must point to a void function accepting two parameters: an integer and a float; for instance:
```
void myCallback(int pin, float value)
```
This function will be called every time the specified pin changes value of an ammount equal or bigger than the `minVariation` parameter and mantains such difference at least for a time equal to the `stableTime` parameter, in milliseconds.
The parameters passed to the callback function will correspond to the monitored pin and the value that triggered the call.

### Iono.process()
This method must be called periodically (inside the `loop()` function) if `subscribeDigital()` or `subscribeAnalog()` are used.

This method checks the inputs and calls the callback functions if required.
