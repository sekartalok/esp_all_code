#include <ESP32DMASPIMaster.h>
#include <Arduino.h>
#include "ICM2048DMA.h"


// HSPI pins
#define PIN_SCK   12
#define PIN_MISO  35
#define PIN_MOSI  36
#define PIN_CS    34


ICM20948_DMA icm(PIN_CS, PIN_SCK, PIN_MISO, PIN_MOSI);

void setup() {
  Serial.begin(9600);
  delay(500);
  Serial.println("Initializing ICM20948...");

  if (icm.init()) {
    Serial.println("ICM20948 initialized successfully!");
  } else {
    Serial.println("ICM20948 initialization failed!");
  }

  // Test WHO_AM_I
  uint8_t whoami = icm.whoAmI();
  Serial.print("WHO_AM_I: 0x");
  Serial.println(whoami, HEX);


}

void loop() {

}
