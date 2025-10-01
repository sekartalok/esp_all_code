#include "ICM2048DMA.h"

ICM20948_DMA::ICM20948_DMA(int cs, int sck, int miso, int mosi)
: csPin(cs), sckPin(sck), misoPin(miso), mosiPin(mosi)
{
    master = new ESP32DMASPI::Master();
}

ICM20948_DMA::~ICM20948_DMA() {
    if (dma_tx_buf) heap_caps_free(dma_tx_buf);
    if (dma_rx_buf) heap_caps_free(dma_rx_buf);
    if (master) delete master;
}

/************ Basic Settings ************/ 
void ICM20948_DMA::switchBank(uint8_t newBank) {
    if (newBank != currentBank) {
        currentBank = newBank;
        
        // Prepare bank switch command
        dma_tx_buf[0] = static_cast<uint8_t>(ICM20948_All_Bank::ICM20948_REG_BANK_SEL) & 0x7F;
        dma_tx_buf[1] = newBank << 4;
        
        // Execute DMA transfer (2 bytes: register + data)
        digitalWrite(csPin, LOW);
        master->transfer(dma_tx_buf, nullptr, 2);
        digitalWrite(csPin, HIGH);
        delayMicroseconds(10);  // Bank switch settling time
    }
}

void ICM20948_DMA::reset_ICM20948() {
    // Switch to Bank 0 (PWR_MGMT_1 is in Bank 0)
    switchBank(0);
    
    // Prepare reset command in DMA buffer
    dma_tx_buf[0] = ICM20948_PWR_MGMT_1 & 0x7F;  // Write operation (MSB=0)
    dma_tx_buf[1] = static_cast<uint8_t>(REGISTER_BITS::ICM20948_RESET);  // 0x80 reset bit
    
    // Execute blocking DMA transfer (2 bytes: register + data)
    digitalWrite(csPin, LOW);
    master->transfer(dma_tx_buf, nullptr, 2);
    digitalWrite(csPin, HIGH);   
    delay(10);  // Wait for internal registers to reset
}


void ICM20948_DMA::writeRegister8(uint8_t bank, uint8_t reg, uint8_t val) {
    // Switch to correct register bank first
    switchBank(bank);
    
    // Prepare 2-byte command in DMA buffer
    dma_tx_buf[0] = reg & 0x7F;  // Clear MSB for write operation
    dma_tx_buf[1] = val;
    
    // Execute blocking DMA transfer
    digitalWrite(csPin, LOW);
    master->transfer(dma_tx_buf, nullptr, 2);
    digitalWrite(csPin, HIGH);
    delayMicroseconds(5);  // Register write settling time
}


uint8_t ICM20948_DMA::readRegister8(uint8_t bank, uint8_t reg) {
    switchBank(bank);
    
    // Prepare DMA buffers
    dma_tx_buf[0] = reg | 0x80;  // Set read bit (MSB = 1)
    dma_tx_buf[1] = 0x00;        // Dummy byte for receiving data
    
    // Execute DMA SPI transfer
    digitalWrite(csPin, LOW);
    master->transfer(dma_tx_buf, dma_rx_buf, 2);
    digitalWrite(csPin, HIGH);
    
    delayMicroseconds(10);  // Small delay for sensor timing
    
    return dma_rx_buf[1];  // Return received data (second byte)
}

uint8_t ICM20948_DMA::whoAmI() {
    return readRegister8(0, ICM20948_WHO_AM_I);
}

bool ICM20948_DMA::init() {
    // Configure CS pin
#ifndef PIN_CS_ICM
#define PIN_CS_ICM
    pinMode(csPin, OUTPUT);
#endif
    digitalWrite(csPin, HIGH);

    // Allocate DMA buffers
    dma_tx_buf = master->allocDMABuffer(256);
    dma_rx_buf = master->allocDMABuffer(256);

    // Configure SPI parameters BEFORE begin()
    master->setDataMode(SPI_MODE0);           // ICM20948 uses Mode 0
    master->setFrequency(7000000);            // 7 MHz max for ICM20948
    master->setMaxTransferSize(256);
    master->setQueueSize(1);

    // Initialize SPI with pins
    master->begin(HSPI, sckPin, misoPin, mosiPin, csPin);

    delay(100);

    // Reset sensor and select bank 0
    currentBank = 0;
    //reset_ICM20948();

    // Retry WHO_AM_I up to 10 times
    uint8_t tries = 0;
    while (whoAmI() != static_cast<uint8_t>(OTHERS::ICM20948_WHO_AM_I_CONTENT) && tries < 10) {
        reset_ICM20948();
        delay(300);
        tries++;
    }
    if (tries == 10) {
        return false;
    }

    // Initialize offsets and scaling
    accOffsetVal = {0.0f, 0.0f, 0.0f};
    gyrOffsetVal = {0.0f, 0.0f, 0.0f};
    accRangeFactor = 1;
    gyrRangeFactor = 1;

    // Wake up and align ODR
    //sleep(false);
    writeRegister8(2, static_cast<uint8_t>(ICM20948_Bank_2_Registers::ODR_ALIGN_EN), 1);

    return true;
}
