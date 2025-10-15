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
    while(true);
  }
  if(imu.init_AK09916()){
    Serial.println("AK WORKING");
  }else{
    Serial.println("AK NOT WORKING");
  }
imu.dmaEnable();
delay(10);

}

void loop() {
  Serial.println("READ SENSOR");
  xyzFloat magValue;
  imu.readSensor();
  imu.getMagValues(&magValue);
  Serial.println("Magnetometer Data in µTesla: ");
  Serial.print(magValue.x);
  Serial.print("   ");
  Serial.print(magValue.y);
  Serial.print("   ");
  Serial.println(magValue.z);
    
  delay(1000);

}


