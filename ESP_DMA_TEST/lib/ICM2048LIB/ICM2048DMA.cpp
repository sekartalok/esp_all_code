#include "ICM2048DMA.h"

ICM20948_DMA::ICM20948_DMA(int sck, int miso, int mosi, int cs) {
    dma_tx = nullptr;
    dma_rx = nullptr;
    master.begin(HSPI, sck, miso, mosi, cs);
}

ICM20948_DMA::~ICM20948_DMA() {
    if (dma_tx) free(dma_tx);
    if (dma_rx) free(dma_rx);
}

void ICM20948_DMA::spiTransfer(size_t len) {
    size_t aligned_len = (len + 3) & ~0x03; // 4-byte align for SPI DMA
    master.queue(dma_tx, dma_rx, aligned_len);
    master.trigger();
}

uint8_t ICM20948_DMA::WhoimI() {
    return readRegister8(0, WHO_AM_I);
}

void ICM20948_DMA::switchBank(uint8_t newBank) {
    if (newBank != currentBank) {
        currentBank = newBank;
        dma_tx[0] = REG_BANK_SEL & 0x7F;
        dma_tx[1] = (currentBank & 0x03) << 4;
        spiTransfer(2);
    }
}

void ICM20948_DMA::writeRegister8(uint8_t bank, uint8_t reg, uint8_t val) {
    switchBank(bank);
    dma_tx[0] = reg & 0x7F;
    dma_tx[1] = val;
    spiTransfer(2);
}

uint8_t ICM20948_DMA::readRegister8(uint8_t bank, uint8_t reg) {
    switchBank(bank);
    dma_tx[0] = reg | 0x80;
    dma_tx[1] = 0x00;
    spiTransfer(2);
    return dma_rx[1];
}

void ICM20948_DMA::readRegisters(uint8_t bank, uint8_t reg, uint8_t *buf, size_t len) {
    switchBank(bank);
    dma_tx[0] = reg | 0x80;
    for (size_t i = 1; i <= len; i++) dma_tx[i] = 0x00;
    spiTransfer(len + 1);
    for (size_t i = 0; i < len; i++) buf[i] = dma_rx[i + 1];
}

bool ICM20948_DMA::init_ICM20948() {
    writeRegister8(0, PWR_MGMT_1, RESET);
    delay(50);

    uint8_t who = WhoimI();
    if (who != WHO_AM_I_VALUE) {
        Serial.printf("WHO_AM_I mismatch: 0x%02X\n", who);
        return false;
    }
    Serial.printf("WORKING: 0x%02X\n", who);

    uint8_t pwr_mgmt_1 = readRegister8(0, PWR_MGMT_1);
    writeRegister8(0, PWR_MGMT_1, pwr_mgmt_1 & ~SLEEP);

    writeRegister8(2, ODR_ALIGN_EN, 1);

    // Enable the magnetometer
    return enableMagnetometer();
}

bool ICM20948_DMA::begin() {
    dma_tx = master.allocDMABuffer(64);
    dma_rx = master.allocDMABuffer(64);

    master.setDataMode(SPI_MODE0);
    master.setFrequency(1000000);
    master.setMaxTransferSize(64);
    master.setQueueSize(1);

    delay(100);

    return init_ICM20948();
}

bool ICM20948_DMA::enableMagnetometer() {
    // Enable I2C master interface
    writeRegister8(0, USER_CTRL, I2C_MST_EN);
    delay(10);

    // I2C Master clock config
    writeRegister8(3, I2C_MST_CTRL, 0x07);  // 345.6 kHz I2C speed

    // AK09916 I2C address (0x0C)
    // Configure slave 0 for write
    writeRegister8(3, I2C_SLV0_ADDR, 0x0C);  // write mode
    writeRegister8(3, I2C_SLV0_REG, 0x31);   // CNTL2 register of AK09916
    writeRegister8(3, I2C_SLV0_DO, 0x08);    // Continuous measurement mode 2 (100 Hz)
    writeRegister8(3, I2C_SLV0_CTRL, 0x81);  // Enable, 1 byte

    delay(50);

    // Setup slave 0 to read AK09916 data
    writeRegister8(3, I2C_SLV0_ADDR, 0x8C);  // Read mode (bit7=1)
    writeRegister8(3, I2C_SLV0_REG, 0x11);   // Start at ST1 register
    writeRegister8(3, I2C_SLV0_CTRL, 0x88);  // Enable, 8 bytes (ST1 + XYZ + ST2)

    delay(50);

    return true;
}

void ICM20948_DMA::readAllData(uint8_t *data) {
    switchBank(0);
    dma_tx[0] = ACCEL_OUT | 0x80;
    for (int i = 1; i <= 20; i++) dma_tx[i] = 0x00;
    spiTransfer(21);

    for (int i = 0; i < 20; i++) {
        data[i] = dma_rx[i + 1];
    }

    ax = (data[0] << 8) | data[1];
    ay = (data[2] << 8) | data[3];
    az = (data[4] << 8) | data[5];
    temp = (data[6] << 8) | data[7];
    gx = (data[8] << 8) | data[9];
    gy = (data[10] << 8) | data[11];
    gz = (data[12] << 8) | data[13];

    // Read magnetometer data from EXT_SLV_SENS_DATA_00
    uint8_t magBuf[8];
    readRegisters(0, EXT_SLV_SENS_DATA_00, magBuf, 8);

 mx = (magBuf[1] << 8) | magBuf[0];
 my = (magBuf[3] << 8) | magBuf[2];
 mz = (magBuf[5] << 8) | magBuf[4];
}
