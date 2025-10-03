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

constexpr uint8_t ICM20948_GYRO_XOUT_H    = 0x33;
constexpr uint8_t ICM20948_GYRO_XOUT_L    = 0x34;
constexpr uint8_t ICM20948_GYRO_YOUT_H    = 0x35;
constexpr uint8_t ICM20948_GYRO_YOUT_L    = 0x36;
constexpr uint8_t ICM20948_GYRO_ZOUT_H    = 0x37;
constexpr uint8_t ICM20948_GYRO_ZOUT_L    = 0x38;

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
    size_t aligned_len = (len + 3) & ~0x03; // align to 4
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
// Gyro Read
//====================================================================================
void readGyro(int16_t &gx, int16_t &gy, int16_t &gz) {
    uint8_t raw[6];
    readRegisters(0, ICM20948_GYRO_XOUT_H, raw, 6);

    gx = (int16_t)((raw[0] << 8) | raw[1]);
    gy = (int16_t)((raw[2] << 8) | raw[3]);
    gz = (int16_t)((raw[4] << 8) | raw[5]);
}

//====================================================================================
// Arduino
//====================================================================================
void setup() {
    Serial.begin(115200);

    dma_tx = master.allocDMABuffer(16);
    dma_rx = master.allocDMABuffer(16);

    master.setDataMode(SPI_MODE0);
    master.setFrequency(1000000);   // safe startup
    master.setMaxTransferSize(16);
    master.setQueueSize(1);

    master.begin(HSPI, SCL, ADO, SDA, NCS);

    delay(100);

    if (init_ICM20948()) {
        Serial.println("ICM20948 READY ✅");
    } else {
        Serial.println("ICM20948 FAIL ❌");
        while (1) delay(1000);
    }
}

void loop() {
    int16_t gx, gy, gz;
    readGyro(gx, gy, gz);

    Serial.printf("Gyro X:%d  Y:%d  Z:%d\n", gx, gy, gz);
    delay(500);
}
