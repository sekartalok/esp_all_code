#include <Arduino.h>
#include "ICM2048DMA.h"


#define SCL  12   // SCK
#define SDA  35   // MOSI
#define NCS  34   // CS
#define ADO  36   // MISO


ICM20948_DMA sensor(SCL, ADO, SDA, NCS);


void setup() {
    Serial.begin(9600);
    
    

   
    if(sensor.init()){
       Serial.println("WORKING");
        
    }else{
       Serial.println("NOT WORKING");
    }

      Serial.println("A");

}

void loop() {

}
