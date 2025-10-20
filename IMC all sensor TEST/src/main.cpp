#include "GPIOIMC.h"
#include <ICM20948_WE.h> // Assuming this is the header for the library

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
  

  
  my_sensor.enableInterrupt(ICM20948_DATA_READY_INT);
  attachInterrupt(digitalPinToInterrupt(intPin),dataReadyISR,RISING);
 // my_sensor.readAndClearInterrupts(); // Clear any pending interrupts at startup


   //my_sensor.setIntPinPolarity(ICM20948_ACT_LOW);
  my_sensor.enableIntLatch(true);
  
  // ADD THIS LINE to enable clearing the interrupt on any register read
  my_sensor.enableClearIntByAnyRead(true); 

  my_sensor.readAndClearInterrupts();
}

void loop() {
  if(dataReady){
    dataReady = false; // Reset the flag immediately
    
    // This function now reads data AND clears the interrupt
    gyro(); 
    
    // The manual readAndClearInterrupts() calls are no longer needed
  }
}

void init_gyro(){
  my_sensor.setGyrRange(ICM20948_GYRO_RANGE_250);
  my_sensor.setGyrDLPF(ICM20948_DLPF_7);   
}

void gyro(){
  xyzFloat gyrRaw; 
  
  // This call will read the sensor data and, because of the new setting,
  // it will also clear the DATA_READY_INT status.
  my_sensor.readSensor(); 
  
  my_sensor.getGyrRawValues(&gyrRaw);
  
  Serial.println("Raw gyroscope values (x,y,z):");
  Serial.print(gyrRaw.x);
  Serial.print("   ");
  Serial.print(gyrRaw.y);
  Serial.print("   ");
  Serial.println(gyrRaw.z);
  Serial.println();
}