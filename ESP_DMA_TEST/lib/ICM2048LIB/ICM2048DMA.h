#ifndef ICM_2048
#define ICM_2048

#include <Arduino.h>
#include "ESP32DMASPIMaster.h"

using namespace ESP32DMASPI;

class ICM20948_DMA {
public:
    ICM20948_DMA(int sck, int miso, int mosi, int cs);
    ~ICM20948_DMA();

    bool begin();                   // init sensor
    void readAllData(uint8_t *data); // burst read accel/gyro/temp/mag

    // convenience getters
    float getAccelX_g() { return ax / 16384.0f; }
    float getAccelY_g() { return ay / 16384.0f; }
    float getAccelZ_g() { return az / 16384.0f; }

    float getGyroX_dps() { return gx / 131.0f; }
    float getGyroY_dps() { return gy / 131.0f; }
    float getGyroZ_dps() { return gz / 131.0f; }

    float getTemperature_C() { return (temp / 333.87f) + 21.0f; }

    int16_t getMagX() { return mx; }
    int16_t getMagY() { return my; }
    int16_t getMagZ() { return mz; }
    uint8_t WhoimI();

private:
    Master master;
    uint8_t *dma_tx, *dma_rx;
    uint8_t currentBank = 0xFF;

    // raw data
    int16_t ax, ay, az;
    int16_t gx, gy, gz;
    int16_t mx, my, mz;
    int16_t temp;

    // registers
    static constexpr uint8_t REG_BANK_SEL   = 0x7F;
    static constexpr uint8_t WHO_AM_I       = 0x00;
    static constexpr uint8_t PWR_MGMT_1     = 0x06;
    static constexpr uint8_t WHO_AM_I_VALUE = 0xEA;
    static constexpr uint8_t ACCEL_OUT      = 0x2D;
    static constexpr uint8_t ODR_ALIGN_EN   = 0x09;
    static constexpr uint8_t RESET          = 0x80;
    static constexpr uint8_t SLEEP          = 0x40;

    // low level
    
    void spiTransfer(size_t len);
    void switchBank(uint8_t newBank);
    void writeRegister8(uint8_t bank, uint8_t reg, uint8_t val);
    uint8_t readRegister8(uint8_t bank, uint8_t reg);
    void readRegisters(uint8_t bank, uint8_t reg, uint8_t *buf, size_t len);

    bool init_ICM20948();


    bool enableMagnetometer();

    static constexpr uint8_t USER_CTRL           = 0x03;
    static constexpr uint8_t I2C_MST_EN          = 0x20;
    static constexpr uint8_t I2C_MST_CTRL        = 0x01;
    static constexpr uint8_t I2C_SLV0_ADDR       = 0x03;
    static constexpr uint8_t I2C_SLV0_REG        = 0x04;
    static constexpr uint8_t I2C_SLV0_CTRL       = 0x05;
    static constexpr uint8_t I2C_SLV0_DO         = 0x06;
    static constexpr uint8_t EXT_SLV_SENS_DATA_00= 0x3B; // or 0x3B (check datasheet)
};

#endif
