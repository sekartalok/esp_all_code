#include <Arduino.h>
#include <string>
#include "sd_lib.h"

#include "ICM2048DMA.h"

#define SCL  12
#define SDA  35
#define NCS  34
#define ADO  36

#define inter 48

using namespace std;
static const int maxs = 1000;
xyzFloat mag[maxs];
xyzFloat acc[maxs];
xyzFloat gyr[maxs];
SD_LIB my_sd;

ICM20948_DMA imu(SCL, ADO, SDA, NCS);

volatile bool ready{false};
void IRAM_ATTR Testinterupt(){
  ready = true;
}

void read(int i){
  imu.readSensorDMA();
  imu.getAccRawValues(&acc[i]);
  imu.getGyrRawValues(&gyr[i]);
  imu.getMagValues(&mag[i]);
  Serial.println(acc[i].z);
}

void sensor_master(){
  int i=0;
  while(i < maxs){
  if(ready){
  Serial.println("READ");
  imu.readAndClearInterruptDMA();
  read(i);
  ready = false;
  imu.readAndClearInterruptDMA();
  i++;
  }
  }

  
}

void write_master(){
  Serial.println("WRITE");
  int i =0;
  my_sd.nano("/ICM_SENSOR_Z/ICM.txt","MAGNETO",1);
  while(i < maxs){
    my_sd.nano("/ICM_SENSOR_Z/ICM.txt",to_string(mag[i].z).c_str(),1);
    i++;
  }
  i = 0;
  my_sd.nano("/ICM_SENSOR_Z/ICM.txt","ACCE",1);
  while(i < maxs){
    my_sd.nano("/ICM_SENSOR_Z/ICM.txt",to_string(acc[i].z).c_str(),1);
    i++;
  }
  i = 0;
  my_sd.nano("/ICM_SENSOR_Z/ICM.txt","GYRO",1);
   while(i < maxs){
    my_sd.nano("/ICM_SENSOR_Z/ICM.txt",to_string(gyr[i].z).c_str(),1);
    i++;
  }
  

}




void setup() {
  Serial.begin(115200);
  


  if(imu.allInit()){
    Serial.println("WORKING");
  }else{
    Serial.println("NOT WORKING");
    while(1);
    
  }
  imu.setIntPinPolarity(ICM20948_ACT_LOW);
  imu.enableIntLatch(true);
  imu.enableDataRedyInterrupt();

  attachInterrupt(digitalPinToInterrupt(inter),Testinterupt,FALLING);
  imu.readAndClearInterrupts();



  if (my_sd.sd_begin(SD)){
    Serial.println("SD FAIL");
    while(1);
  }else{
    Serial.println("WORKING");
    Serial.println(my_sd.ls().c_str());
  }

  my_sd.rf("/ICM_SENSOR_Z/ICM.txt");
  my_sd.rmdir("/ICM_SENSOR_Z");
  my_sd.mkdir("/ICM_SENSOR_Z");
  my_sd.touch("/ICM_SENSOR_Z/ICM.txt");


  sensor_master();
  write_master();

  Serial.println(my_sd.echo("/ICM_SENSOR_Z/ICM.txt").c_str());

  



}

void loop() {
  // put your main code here, to run repeatedly:
}

