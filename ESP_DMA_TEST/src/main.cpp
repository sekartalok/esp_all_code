#include <Arduino.h>
#include "ICM2048DMA.h"

#define SCL  12
#define SDA  35
#define NCS  34
#define ADO  36

ICM20948_DMA imu(SCL, ADO, SDA, NCS);

void setup() {
    Serial.begin(115200);

    if (imu.begin()) {
        Serial.println("ICM20948 READY");
    } else {
        Serial.println("ICM20948 FAIL");
        while (1) delay(1000);
        
    }
   
}

void loop() {
    Serial.println(imu.WhoimI());
    uint8_t raw[20];
    imu.readAllData(raw);

    Serial.printf("ACCEL[g]: X=%.2f Y=%.2f Z=%.2f | ",
        imu.getAccelX_g(), imu.getAccelY_g(), imu.getAccelZ_g());

    Serial.printf("GYRO[dps]: X=%.2f Y=%.2f Z=%.2f | ",
        imu.getGyroX_dps(), imu.getGyroY_dps(), imu.getGyroZ_dps());

    Serial.printf("TEMP: %.2f °C | ", imu.getTemperature_C());
    Serial.printf("MAG raw: X=%d Y=%d Z=%d\n", imu.getMagX(), imu.getMagY(), imu.getMagZ());

    delay(500);
}
