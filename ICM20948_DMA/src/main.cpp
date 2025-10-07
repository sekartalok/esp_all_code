#include <Arduino.h>
#include "ICM2048DMA.h"

#define SCL  12
#define SDA  35
#define NCS  34
#define ADO  36

ICM20948_DMA imu(SCL, ADO, SDA, NCS);

void setup() {
  Serial.begin(115200);
  if(imu.init()){
    Serial.println("WORKING");
  }else{
    Serial.println("not working");
  }

}

void loop() {

}


