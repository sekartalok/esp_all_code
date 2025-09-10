
#include "sd_lib.h"

SD_LIB my_sd;




















void setup() {
Serial.begin(9600);

my_sd.sd_begin(SD);
Serial.println(my_sd.ls().c_str());







  //ls(SD,root).c_str();

}

void loop() {
  // put your main code here, to run repeatedly:
}

