#include <Arduino.h>
#include "ICM2048DMA.h"


#define SCL  12   // SCK
#define SDA  35   // MOSI
#define NCS  34   // CS
#define ADO  36   // MISO

ICM20948_DMA master(&SPI,NCS);



void setup() {
Serial.begin(9600);
while(!Serial);
SPI.begin(SCL, ADO, SDA, NCS);

if(!master.init()){Serial.println("NOT WORKING");}
else{Serial.println("WORKING");}

}

void loop() {
}

