#include <Iono.h>

void setup() {
  Serial.begin(9600);
  
  /* 
  / Call onDI1Change() every time
  / DI1 changes value and is stable
  / for 500ms
  */
  Iono.subscribeDigital(DI1, 500, &onDI1Change);
  
  /*
  / Call onAV2Change() every time 
  / the voltage on AV2 changes of
  / a value >= 1V, without any delay
  */
  Iono.subscribeAnalog(AV2, 0, 1, &onAV2Change);
}

void loop() {
  // Check all the inputs
  Iono.process();
  delay(100);
}

void onDI1Change(int pin, float value) {
  Serial.print("DI1 switched ");
  Serial.println(value == HIGH ? "on" : "off");
}

void onAV2Change(int pin, float value) {
  Serial.print("AV2 voltage: ");
  Serial.println(value);
}
