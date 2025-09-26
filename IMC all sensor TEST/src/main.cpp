#include "GPIOIMC.h"


#if spi == 1
ICM20948_WE my_sensor(&SPI, NCS, spi);
#endif

//accelerator
void init_accelerator();
void accelerator();

//gyro
void init_gyro();
void gyro();

//magnet
void init_magnet();
void magnet();


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
  init_accelerator();
  init_gyro();
  init_magnet();
}

void loop() {
  gyro();
  accelerator();
  magnet();
}

void init_accelerator(){
  Serial.println("DONT MOVE CALLIBRATIONS.....");
  my_sensor.autoOffsets();
  Serial.println("done calibrating");
  my_sensor.setAccRange(ICM20948_ACC_RANGE_2G);
  my_sensor.setAccDLPF(ICM20948_DLPF_6);
  my_sensor.setAccSampleRateDivider(10);

}
void accelerator(){
  xyzFloat accRaw;
  xyzFloat corrAccRaw;
  xyzFloat gVal;

  my_sensor.readSensor();
  my_sensor.getAccRawValues(&accRaw);
  my_sensor.getCorrectedAccRawValues(&corrAccRaw);
  my_sensor.getGValues(&gVal);
  float resultantG = my_sensor.getResultantG(&gVal);
   
  Serial.println("Raw acceleration values (x,y,z):");
  Serial.print(accRaw.x);
  Serial.print("   ");
  Serial.print(accRaw.y);
  Serial.print("   ");
  Serial.println(accRaw.z);
  Serial.println("Corrected raw acceleration values (x,y,z):");
  Serial.print(corrAccRaw.x);
  Serial.print("   ");
  Serial.print(corrAccRaw.y);
  Serial.print("   ");
  Serial.println(corrAccRaw.z);
  Serial.println("g-values (x,y,z):");
  Serial.print(gVal.x);
  Serial.print("   ");
  Serial.print(gVal.y);
  Serial.print("   ");
  Serial.println(gVal.z);
  Serial.print("Resultant g: ");
  Serial.println(resultantG);
  Serial.println("*************************************");
 
  delay(1000);

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
void init_magnet(){
  if(!my_sensor.initMagnetometer()){
    Serial.println("magnetor fail to innit");
  }
  Serial.println("magnetor on");

  my_sensor.setMagOpMode(AK09916_CONT_MODE_20HZ);
}

void magnet(){
  xyzFloat magValue; // x/y/z magnetic flux density [µT] 
  my_sensor.readSensor();
  my_sensor.getMagValues(&magValue);
  Serial.println("Magnetometer Data in µTesla: ");
  Serial.print(magValue.x);
  Serial.print("   ");
  Serial.print(magValue.y);
  Serial.print("   ");
  Serial.println(magValue.z);
  delay(1000);
}