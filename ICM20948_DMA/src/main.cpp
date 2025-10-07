#include <Arduino.h>
#include "ICM2048DMA.h"

#define SCL  12
#define SDA  35
#define NCS  34
#define ADO  36

ICM20948_DMA imu(SCL, ADO, SDA, NCS);

void setup() {
  Serial.begin(115200);
  if (imu.init()) Serial.println("ICM20948 working ✅");
  if (imu.init_AK09916()) Serial.println("MAGNET WORKING ✅");
  else Serial.println("MAGNET FAIL ❌");

  Serial.print("WHO_AM_I_AK09916 = 0x");
  Serial.println(imu.whoAmI_AK09916(), HEX);

}

void loop() {

}


