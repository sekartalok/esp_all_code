#include "GPIOIMC.h"


#if spi == 1
ICM20948_WE my_sensor(&SPI, NCS, spi);
#endif

const int intPin = 48;
volatile bool dataReady{false};

//gyro
void init_gyro();
void gyro();

void IRAM_ATTR dataReadyISR() {
  dataReady = true;
}



void setup() {
  Serial.begin(115200);
  delay(500);

  #if spi == 1
  SPI.begin(SCL, ADO, SDA, NCS);
  #endif

  if (my_sensor.init()) {
    Serial.println("WORKING");
  } else {
    Serial.println("NOT WORKING");
    while(1);  // Halt if sensor init fails
  }
  init_gyro();
  //my_sensor.setIntPinPolarity(ICM20948_ACT_LOW);
  my_sensor.enableIntLatch(true);
  my_sensor.enableInterrupt(ICM20948_DATA_READY_INT);
  attachInterrupt(digitalPinToInterrupt(intPin),dataReadyISR,RISING);
  my_sensor.readAndClearInterrupts();


}

void loop() {
  if(dataReady){
    Serial.println( my_sensor.readAndClearInterrupts());
    gyro();
    dataReady = false;
    my_sensor.readAndClearInterrupts();
  }
}



void init_gyro(){

  Serial.println("DONT MOVE CALLIBRATIONS.....");
  my_sensor.autoOffsets();
  Serial.println("done calibrating");
  my_sensor.setGyrRange(ICM20948_GYRO_RANGE_250);
  my_sensor.setGyrDLPF(ICM20948_DLPF_6);  

}
void gyro(){
  xyzFloat gyrRaw; 
  xyzFloat gyr;
  my_sensor.readSensor();
  my_sensor.getCorrectedGyrRawValues(&gyrRaw);
  my_sensor.getGyrValues(&gyr);
    
  Serial.println("Raw gyroscope values (x,y,z):");
  Serial.print(gyrRaw.x);
  Serial.print("   ");
  Serial.print(gyrRaw.y);
  Serial.print("   ");
  Serial.println(gyrRaw.z);
  Serial.println("Gyroscope values (x,y,z):");
  Serial.print(gyr.x);
  Serial.print("   ");
  Serial.print(gyr.y);
  Serial.print("   ");
  Serial.println(gyr.z);
  Serial.println();
  delay(500);
}
