#include "ICM2048DMA.h"


/************ Basic Settings ************/ 

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

        master->transfer(static_cast<uint8_t>(ICM20948_All_Bank::ICM20948_REG_BANK_SEL));
        master->transfer(newBank << 4);
        
        master->endTransaction();
        digitalWrite(csPin, HIGH);

        delay(5);
    }
}

void ICM20948_DMA::reset_ICM20948() {
    writeRegister8(0,ICM20948_PWR_MGMT_1,static_cast<uint8_t>(REGISTER_BITS::ICM20948_RESET));
    delay(5);
}

void ICM20948_DMA::writeRegister8(uint8_t bank, uint8_t reg, uint8_t val) {

    switchBank(bank);

    digitalWrite(csPin,LOW);
    master->beginTransaction(spi_setting);

    master->transfer(reg & ICM20948_WRITE_MASKING_BIT);
    master->transfer(val);

    master->endTransaction();
    digitalWrite(csPin,HIGH);
}


uint8_t ICM20948_DMA::readRegister8(uint8_t bank, uint8_t reg) {
    switchBank(bank);
    
    uint8_t dummy =0x00;

    digitalWrite(csPin,LOW);
    master->beginTransaction(spi_setting);

    master->transfer(reg | ICM20948_READ_MASKING_BIT);
    dummy = master->transfer(0x00);

    master->endTransaction();
    digitalWrite(csPin,HIGH);
    
    return dummy;  
}

uint8_t ICM20948_DMA::whoAmI() {
    return readRegister8(0, ICM20948_WHO_AM_I);
}

bool ICM20948_DMA::init() {
    //ready the CS PIN
    pinMode(csPin, OUTPUT);
    digitalWrite(csPin, HIGH);

    Serial.print("TEST");
    //set the spi setting on speed
    spi_setting = SPISettings(7000000, MSBFIRST, SPI_MODE0);   

    //set current bank to default
    currentBank = 0x00;
   
    reset_ICM20948();
    delay(5);

    uint8_t tries = 0;
    while ((whoAmI() != static_cast<uint8_t>(OTHERS::ICM20948_WHO_AM_I_CONTENT)) && (tries < 5)) {
        reset_ICM20948();
        delay(200);
        Serial.println("HELLO");
        tries++;
    }
    if (tries == 5 ) {
        return false;
    }

    // Initialize offsets and scaling
    accOffsetVal = {0.0f, 0.0f, 0.0f};
    gyrOffsetVal = {0.0f, 0.0f, 0.0f};
    accRangeFactor = 1;
    gyrRangeFactor = 1;

    // Wake up and align ODR
    sleep(false);
    writeRegister8(2, static_cast<uint8_t>(ICM20948_Bank_2_Registers::ODR_ALIGN_EN), 1);

    return true;
}
