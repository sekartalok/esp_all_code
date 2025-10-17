#include <Arduino.h>
#include "ICM2048DMA.h"

#define SCL  12
#define SDA  35
#define NCS  34
#define ADO  36

#define inter 48

ICM20948_DMA imu(SCL, ADO, SDA, NCS);

volatile bool ready{false};
void IRAM_ATTR Testinterupt(){
  ready = true;
}

void read(){
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
}

void setup() {
  Serial.begin(115200);


  if(imu.allInit()){
    Serial.println("WORKING");
  }else{
    Serial.println("NOT WORKING");
    while(1);
    
  }





 // Serial.println(imu.recycle());
 // while(1);
 
  //imu.init_AK09916();
  //imu.end();
  //Serial.println(imu.recycle());
  imu.setIntPinPolarity(ICM20948_ACT_LOW);
  imu.enableIntLatch(true);
  imu.enableDataRedyInterrupt();

 //imu.disableOtherInterrupt();
  attachInterrupt(digitalPinToInterrupt(inter),Testinterupt,FALLING);
  imu.readAndClearInterrupts();


  

}

void loop() {
 
  if(ready){
  imu.readAndClearInterrupts();
  read();
  ready = false;
  imu.readAndClearInterrupts();
   

  }

}


