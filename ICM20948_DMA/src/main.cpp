#include <Arduino.h>
#include "ICM2048DMA.h"

#define SCL  12
#define SDA  35
#define NCS  34
#define ADO  36

ICM20948_DMA imu(SCL, ADO, SDA, NCS);

void setup() {
  Serial.begin(115200);
  if(imu.allInit()){
    Serial.println("WORKING");
    
    
  }else{
    
    Serial.println("not working");
    while(true);
  }


  
delay(10);

}

void loop() {
  Serial.println("READ SENSOR");

  xyzFloat mag;
  xyzFloat acc;
  xyzFloat gyr;
  imu.readSensor();
  imu.getAccRawValues(&acc);
  imu.getGyrRawValues(&gyr);
  imu.getMagValues(&mag);
  
  Serial.println("MAGNETO");
  Serial.print(mag.x);
  Serial.print("   ");
  Serial.print(mag.y);
  Serial.print("   ");
  Serial.println(mag.z);
  Serial.println("ACC");
  Serial.print(acc.x);
  Serial.print("   ");
  Serial.print(acc.y);
  Serial.print("   ");
  Serial.println(acc.z);
  Serial.println("GYR");
  Serial.print(gyr.x);
  Serial.print("   ");
  Serial.print(gyr.y);
  Serial.print("   ");
  Serial.println(gyr.z);
  delay(1000);


}