#include <Arduino.h>
#include "ICM2048DMA.h"

#define SCL  12
#define SDA  35
#define NCS  34
#define ADO  36

ICM20948_DMA imu(SCL, ADO, SDA, NCS);

void setup() {
  Serial.begin(9600);
  if(imu.init()){
    Serial.println("working");
  }
    

}

void loop() {
  imu.readSensor();
  xyzFloat accRaw;
  imu.getAccRawValues(&accRaw);
  Serial.println("Raw acceleration values (x,y,z):");
  Serial.print(accRaw.x);
  Serial.print("   ");
  Serial.print(accRaw.y);
  Serial.print("   ");
  Serial.println(accRaw.z);
  
  delay(200);
  Serial.println(imu.whoAmI());

}


