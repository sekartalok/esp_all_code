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
    size_t aligned_len = (len + 3) & ~0x03; // 4byte is missing SPI SLAVE
    master.queue(dma_tx, dma_rx, aligned_len);
    master.trigger();
}
uint8_t ICM20948_DMA::WhoimI(){
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
    Serial.printf("WORKING: 0x%02X\n", WhoimI());


    uint8_t pwr_mgmt_1 = readRegister8(0, PWR_MGMT_1);
    writeRegister8(0, PWR_MGMT_1, pwr_mgmt_1 & ~SLEEP);

    writeRegister8(2, ODR_ALIGN_EN, 1);

    return true;
}

bool ICM20948_DMA::begin() {
    dma_tx = master.allocDMABuffer(32);
    dma_rx = master.allocDMABuffer(32);

    master.setDataMode(SPI_MODE0);
    master.setFrequency(1000000);
    master.setMaxTransferSize(32);
    master.setQueueSize(1);

    delay(100);

    return init_ICM20948();
}

void ICM20948_DMA::readAllData(uint8_t *data) {
    switchBank(0);
    dma_tx[0] = ACCEL_OUT | 0x80;
    for (int i = 1; i <= 20; i++) dma_tx[i] = 0x00;

    spiTransfer(21);

    for (int i = 0; i < 20; i++) {
        data[i] = dma_rx[i + 1];
    }

    // parse into class fields
    ax = (data[0] << 8) | data[1];
    ay = (data[2] << 8) | data[3];
    az = (data[4] << 8) | data[5];
    temp = (data[6] << 8) | data[7];
    gx = (data[8] << 8) | data[9];
    gy = (data[10] << 8) | data[11];
    gz = (data[12] << 8) | data[13];
    mx = (data[14] << 8) | data[15];
    my = (data[16] << 8) | data[17];
    mz = (data[18] << 8) | data[19];
}
