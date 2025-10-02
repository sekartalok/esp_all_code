#include "ICM2048DMA.h"

void ICM20948_DMA::sleep(bool sleep) {
    uint8_t temp = readRegister8(0, ICM20948_PWR_MGMT_1);
    if(sleep) {
        temp |= static_cast<uint8_t>(REGISTER_BITS::ICM20948_SLEEP);
    } else {
        temp &= ~static_cast<uint8_t>(REGISTER_BITS::ICM20948_SLEEP);
    }
    writeRegister8(0, ICM20948_PWR_MGMT_1, temp);
}

void ICM20948_DMA::switchBank(uint8_t newBank) {
    if (newBank != currentBank) {
        currentBank = newBank;
        digitalWrite(csPin, LOW);
        master->beginTransaction(spi_setting);
        

        master->transfer(static_cast<uint8_t>(ICM20948_All_Bank::ICM20948_REG_BANK_SEL) & ICM20948_WRITE_MASKING_BIT);
        master->transfer((newBank & ICM20948_BANK_MASKING_BIT ) << 4);

        
        master->endTransaction();
        digitalWrite(csPin, HIGH);

        delayMicroseconds(10);
    }
}

void ICM20948_DMA::reset_ICM20948() {
    writeRegister8(0, ICM20948_PWR_MGMT_1,
                   static_cast<uint8_t>(REGISTER_BITS::ICM20948_RESET));
    delay(100); 
}

void ICM20948_DMA::writeRegister8(uint8_t bank, uint8_t reg, uint8_t val) {
    switchBank(bank);

    digitalWrite(csPin, LOW);
    master->beginTransaction(spi_setting);
    

    master->transfer(reg & ICM20948_WRITE_MASKING_BIT); // write mask
    master->transfer(val);

    
    master->endTransaction();
    digitalWrite(csPin, HIGH);

    delayMicroseconds(5);
}

uint8_t ICM20948_DMA::readRegister8(uint8_t bank, uint8_t reg) {
    switchBank(bank);

    uint8_t value = 0;

    digitalWrite(csPin, LOW);
    master->beginTransaction(spi_setting);
    

    master->transfer(reg | ICM20948_READ_MASKING_BIT); // read mask
    value = master->transfer(0x00);

    
    master->endTransaction();
    digitalWrite(csPin, HIGH);

    delayMicroseconds(5);
    return value;
}

uint8_t ICM20948_DMA::whoAmI() {
    return readRegister8(0, ICM20948_WHO_AM_I);
}

bool ICM20948_DMA::init() {
    pinMode(csPin, OUTPUT);
    digitalWrite(csPin, HIGH);

    spi_setting = SPISettings(7000000, MSBFIRST, SPI_MODE0);

    currentBank = 0xFF; 

    reset_ICM20948();   
    delay(100);

    uint8_t tries = 0;
    uint8_t who = 0;
    while (tries < 5) {
        who = whoAmI();
        if (who == static_cast<uint8_t>(OTHERS::ICM20948_WHO_AM_I_CONTENT)) break;
        delay(100);
        tries++;
    }

    Serial.print("WHO_AM_I = 0x");
    Serial.println(who, HEX);

    if (who != static_cast<uint8_t>(OTHERS::ICM20948_WHO_AM_I_CONTENT)) {
        return false;
    }

    // Offsets and scaling
    accOffsetVal = {0.0f, 0.0f, 0.0f};
    gyrOffsetVal = {0.0f, 0.0f, 0.0f};
    accRangeFactor = 1;
    gyrRangeFactor = 1;

    // Wake and align ODR
    sleep(false);
    writeRegister8(2,
        static_cast<uint8_t>(ICM20948_Bank_2_Registers::ODR_ALIGN_EN), 1);

    return true;
}
