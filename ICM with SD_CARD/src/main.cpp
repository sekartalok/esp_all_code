#include <Arduino.h>
#include "sd_lib.h"

using namespace std;
SD_LIB my_sd;

void setup() {
  Serial.begin(9600);
  Serial.println(my_sd.sd_begin(SD));
  Serial.println(my_sd.ls().c_str());
  my_sd.cd("/System Volume Information");
  Serial.println(my_sd.ls().c_str());
  my_sd.cd("/");
  Serial.println(my_sd.ls().c_str());



}

void loop() {
  // put your main code here, to run repeatedly:
}

