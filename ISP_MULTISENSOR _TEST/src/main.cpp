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

if(!master.init()){Serial.println("NOT WORKING");while(1);}
else{Serial.println("WORKING");}
if(master.init_AK09916()){
    Serial.println("magnet working");
}else{
    Serial.println("notworking");
}

}

void loop() {

    delay(1000);
}

