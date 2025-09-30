#include <Arduino.h>
#include "ICM2048DMA.h"

// Pin Definitions (adjust for your board)


// Create ICM20948 DMA object
ICM20948_DMA imu(0,0, 0, 0);

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n\n=================================");
    Serial.println("ICM20948 DMA SPI Test");
    Serial.println("=================================");
    
    Serial.println("\nPin Configuration:");
    Serial.print("  CS:   "); Serial.println(0);
    Serial.print("  SCK:  "); Serial.println(0);
    Serial.print("  MISO: "); Serial.println(0);
    Serial.print("  MOSI: "); Serial.println(0);
    
    // Initialize ICM20948
    Serial.println("\nInitializing ICM20948...");
    if (imu.init()) {
        Serial.println("✓ ICM20948 initialized successfully!");
        
        // Check WHO_AM_I
        uint8_t whoAmI = imu.whoAmI();
        Serial.print("WHO_AM_I: 0x");
        Serial.println(whoAmI, HEX);
        
        if (whoAmI == 0xEA) {
            Serial.println("✓ WHO_AM_I correct (0xEA)");
        } else {
            Serial.println("✗ WHO_AM_I incorrect!");
        }
        delay(100);
        
    } else {
        Serial.println("✗ ICM20948 initialization failed!");
        Serial.println("Check wiring and power supply");
        while (1) {
            delay(1000);
        }
    }
    
}

void loop() {

}
