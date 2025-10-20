#include <Arduino.h>
#include "ESP32_ESC.h"

ESP32_ESC my_rotor(700,2300);
void setup() {
  my_rotor.begin(16,15,18,36);
  // put your setup code here, to run once:

}

void loop() {

  my_rotor.setRotor0Speed(180);
  my_rotor.setRotor3Speed(0);

  delay(1000);

  my_rotor.setRotor0Speed(0);
  my_rotor.setRotor3Speed(180);
  delay(1000);
  
  // put your main code here, to run repeatedly:
}

