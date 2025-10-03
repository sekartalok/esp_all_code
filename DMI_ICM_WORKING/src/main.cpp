#include <Arduino.h>
#include "ESP32DMASPIMaster.h"
using namespace ESP32DMASPI;

//====================================================================================
// Pin definitions
//====================================================================================
#define SCL  12   // SCK
#define SDA  35   // MOSI
#define NCS  34   // CS
#define ADO  36   // MISO

//====================================================================================
// ICM20948 Registers
//====================================================================================
constexpr uint8_t ICM20948_REG_BANK_SEL   = 0x7F;
constexpr uint8_t ICM20948_WHO_AM_I       = 0x00;
constexpr uint8_t ICM20948_PWR_MGMT_1     = 0x06;
constexpr uint8_t ICM20948_WHO_AM_I_VALUE = 0xEA;

constexpr uint8_t ICM20948_ACCEL_OUT      = 0x2D; 
constexpr uint8_t ICM20948_ODR_ALIGN_EN   = 0x09;

constexpr uint8_t ICM20948_RESET = 0x80;
constexpr uint8_t ICM20948_SLEEP = 0x40;

//====================================================================================
// Globals
//====================================================================================
Master master;
uint8_t *dma_tx, *dma_rx;
uint8_t currentBank = 0xFF;

//====================================================================================
// Low-level SPI helpers
//====================================================================================
void spiTransfer(size_t len) {
    size_t aligned_len = (len + 3) & ~0x03; // align to 4 for DMA
    master.transfer(dma_tx, dma_rx, aligned_len);
}

void switchBank(uint8_t newBank) {
    if (newBank != currentBank) {
        currentBank = newBank;
        dma_tx[0] = ICM20948_REG_BANK_SEL & 0x7F;
        dma_tx[1] = (currentBank & 0x03) << 4;
        spiTransfer(2);
    }
}

void writeRegister8(uint8_t bank, uint8_t reg, uint8_t val) {
    switchBank(bank);
    dma_tx[0] = reg & 0x7F;
    dma_tx[1] = val;
    spiTransfer(2);
}

uint8_t readRegister8(uint8_t bank, uint8_t reg) {
    switchBank(bank);
    dma_tx[0] = reg | 0x80;
    dma_tx[1] = 0x00;
    spiTransfer(2);
    return dma_rx[1];
}

void readRegisters(uint8_t bank, uint8_t reg, uint8_t *buf, size_t len) {
    switchBank(bank);
    dma_tx[0] = reg | 0x80;
    for (size_t i = 1; i <= len; i++) dma_tx[i] = 0x00;
    spiTransfer(len + 1);
    for (size_t i = 0; i < len; i++) buf[i] = dma_rx[i + 1];
}

//====================================================================================
// Init
//====================================================================================
bool init_ICM20948() {
    writeRegister8(0, ICM20948_PWR_MGMT_1, ICM20948_RESET);
    delay(50);

    uint8_t who = readRegister8(0, ICM20948_WHO_AM_I);
    if (who != ICM20948_WHO_AM_I_VALUE) {
        Serial.printf("WHO_AM_I mismatch: 0x%02X\n", who);
        return false;
    }

    // Wake up
    uint8_t pwr_mgmt_1 = readRegister8(0, ICM20948_PWR_MGMT_1);
    writeRegister8(0, ICM20948_PWR_MGMT_1, pwr_mgmt_1 & ~ICM20948_SLEEP);

    // Align ODR
    writeRegister8(2, ICM20948_ODR_ALIGN_EN, 1);

    return true;
}

//====================================================================================
// Read All Data (Accel, Temp, Gyro, Mag)
//====================================================================================
void readAllData(uint8_t *data) {
    switchBank(0);

    dma_tx[0] = ICM20948_ACCEL_OUT | 0x80; // read burst from ACCEL_XOUT_H
    for (int i = 1; i <= 20; i++) dma_tx[i] = 0x00;

    spiTransfer(21);

    for (int i = 0; i < 20; i++) {
        data[i] = dma_rx[i + 1];
    }
}

//====================================================================================
// Arduino
//====================================================================================
void setup() {
    Serial.begin(115200);

    dma_tx = master.allocDMABuffer(32);
    dma_rx = master.allocDMABuffer(32);

    master.setDataMode(SPI_MODE0);
    master.setFrequency(1000000);   // safe startup
    master.setMaxTransferSize(32);
    master.setQueueSize(1);

    master.begin(HSPI, SCL, ADO, SDA, NCS);

    delay(100);

    if (init_ICM20948()) {
        Serial.println("ICM20948 READY");
    } else {
        Serial.println("ICM20948 FAIL");
        while (1) delay(1000);
    }
}

void loop() {
    uint8_t raw[20];
    readAllData(raw);

    int16_t ax = (raw[0] << 8) | raw[1];
    int16_t ay = (raw[2] << 8) | raw[3];
    int16_t az = (raw[4] << 8) | raw[5];
    int16_t temp = (raw[6] << 8) | raw[7];
    int16_t gx = (raw[8] << 8) | raw[9];
    int16_t gy = (raw[10] << 8) | raw[11];
    int16_t gz = (raw[12] << 8) | raw[13];
    int16_t mx = (raw[14] << 8) | raw[15]; // mag only valid if AK09916 enabled
    int16_t my = (raw[16] << 8) | raw[17];
    int16_t mz = (raw[18] << 8) | raw[19];

    // Apply scaling for readable values
    float ax_g = ax / 16384.0f; // ±2g
    float ay_g = ay / 16384.0f;
    float az_g = az / 16384.0f;

    float gx_dps = gx / 131.0f; // ±250 dps
    float gy_dps = gy / 131.0f;
    float gz_dps = gz / 131.0f;

    float temp_c = (temp / 333.87f) + 21.0f;

    Serial.printf("ACCEL[g]: X=%.2f Y=%.2f Z=%.2f | ", ax_g, ay_g, az_g);
    Serial.printf("GYRO[dps]: X=%.2f Y=%.2f Z=%.2f | ", gx_dps, gy_dps, gz_dps);
    Serial.printf("TEMP: %.2f °C | ", temp_c);
    Serial.printf("MAG raw: X=%d Y=%d Z=%d\n", mx, my, mz);

    delay(500);
}
