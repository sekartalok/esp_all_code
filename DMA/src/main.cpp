#include "GPIOIMC.h"
#if spi == 1
ICM20948_WE myIMU(&SPI, NCS, spi);
#endif
volatile bool dataReady = false;

uint64_t last_millis = 0;
//buffer
uint8_t rx_buffer[6];

void IRAM_ATTR interupts();
void data_read();
void dma_read();
void sensor_read();

void setup() {
  SPI.begin(SCL, ADO, SDA, NCS);
  Serial.begin(9600);
  while(!Serial);

  if(!(myIMU.init())){
    Serial.println("NOT WORKING");
    while(true);
  }
  Serial.println("working ...init");
  myIMU.autoOffsets();
  Serial.println("Done");

  myIMU.setGyrRange(ICM20948_GYRO_RANGE_250);
  myIMU.setGyrDLPF(ICM20948_DLPF_6);  
  myIMU.setGyrSampleRateDivider(10);

  myIMU.enableIntLatch(true);
  //myIMU.setFSyncIntPolarity(ICM20948_ACT_LOW);
  myIMU.enableInterrupt(ICM20948_DATA_READY_INT);
  dataReady = false;
  attachInterrupt(digitalPinToInterrupt(INT), interupts, RISING);
  myIMU.readAndClearInterrupts();

}
void loop() {
  if(dataReady){
     byte source = myIMU.readAndClearInterrupts();
    if(myIMU.checkInterrupt(source,ICM20948_DATA_READY_INT)){
      dma_read();
    }
    dataReady = false;
    myIMU.readAndClearInterrupts();
  }

}
void IRAM_ATTR interupts() {
  uint64_t  _current_millis = millis();
  if((_current_millis - last_millis) < 10 ) {
    return;
  }
  dataReady = true;

}
void sensor_read(){
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3));
  digitalWrite(NCS, 0x00);

  SPI.transfer(0xB3);
  SPI.transferBytes(NULL, rx_buffer, 6);

  digitalWrite(NCS,0x01);
  SPI.endTransaction();

}
void dma_read(){
  sensor_read();
  int16_t gyroX = (rx_buffer[0] << 8) | rx_buffer[1];
  int16_t gyroY = (rx_buffer[2] << 8) | rx_buffer[3];
  int16_t gyroZ = (rx_buffer[4] << 8) | rx_buffer[5];

  Serial.print("Gyro X: "); Serial.print(gyroX);
  Serial.print(" Y: "); Serial.print(gyroY);
  Serial.print(" Z: "); Serial.println(gyroZ);

  delay(100);

}


void data_read(){
  xyzFloat gValue;
  xyzFloat gyrValue;
  myIMU.readSensor();
  myIMU.getGValues(&gValue);
  myIMU.getGyrValues(&gyrValue);
      
  Serial.println("g-values (x,y,z):");
  Serial.print(gValue.x);
  Serial.print("   ");
  Serial.print(gValue.y);
  Serial.print("   ");
  Serial.println(gValue.z);
  Serial.println();

}
