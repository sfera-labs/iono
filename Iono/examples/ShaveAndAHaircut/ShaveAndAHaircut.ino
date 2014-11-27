#include <Iono.h>

void setup() {
}

void loop() {
  shaveAndAHaircut();
  delay(2000);
}

void shaveAndAHaircut() {
  Iono.flip(DO1);
  delay(350);
  Iono.flip(DO2);
  delay(100);
  Iono.flip(DO3);
  delay(150);
  Iono.flip(DO4);
  delay(300);
  Iono.flip(DO5);
  delay(600);
  Iono.flip(DO6);
  delay(300);
  Iono.flip(DO1);
}
