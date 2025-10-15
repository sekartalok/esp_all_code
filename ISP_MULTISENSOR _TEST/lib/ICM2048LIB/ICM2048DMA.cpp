#include "ICM2048DMA.h"

/* BASIC */

bool ICM20948_DMA::init() {
    pinMode(csPin, OUTPUT);
    digitalWrite(csPin, HIGH);

    spi_setting = SPISettings(1000000, MSBFIRST, SPI_MODE0);

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

  

    if (who != static_cast<uint8_t>(OTHERS::ICM20948_WHO_AM_I_CONTENT)) {
        return false;
    }

    // range default 
    accRangeFactor = 1;
    gyrRangeFactor = 1;

    // Wake and align ODR
    sleep(false);
    writeRegister8(2, static_cast<uint8_t>(ICM20948_Bank_2_Registers::ODR_ALIGN_EN), 1);

    return true;
}

/* READ AND WRITE MASTER */

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
 


/* utils */
uint8_t ICM20948_DMA::whoAmI() {
    return readRegister8(0,static_cast<uint8_t>(ICM20948_Bank0_Registers::ICM20948_WHO_AM_I));
}

void ICM20948_DMA::setAccDLPF(ICM20948_dlpf dlpf){

    uint8_t temp = readRegister8(2,static_cast<uint8_t>(ICM20948_Bank_2_Registers::ACCEL_CONFIG));

    if(dlpf != ICM20948_DLPF_OFF){
        temp |= 0x01;
        temp &= 0xC7;
        temp |= (dlpf<<3);

    }
    else{
        temp &= 0xFE;
    }
    writeRegister8(2,static_cast<uint8_t>(ICM20948_Bank_2_Registers::ACCEL_CONFIG),temp);

}


/* SENSOR SET RANGE */

void ICM20948_DMA::setAccRange(ICM20948_accRange accRange){

    uint8_t temp = readRegister8(2,static_cast<uint8_t>(ICM20948_Bank_2_Registers::ACCEL_CONFIG));
    temp &= ~(0x06);
    temp |= (accRange<<1);
    writeRegister8(2 , static_cast<uint8_t>(ICM20948_Bank_2_Registers::ACCEL_CONFIG) , temp);
    accRangeFactor = 1<<accRange;


}

void ICM20948_DMA::setGyrRange(ICM20948_gyroRange  gyrRange){

    uint8_t temp = readRegister8(2,static_cast<uint8_t>(ICM20948_Bank_2_Registers::GYRO_CONFIG_1));
    temp &= ~(0x06);
    temp |= (gyrRange<<1);
    writeRegister8(2 , static_cast<uint8_t>(ICM20948_Bank_2_Registers::GYRO_CONFIG_1) , temp);
    gyrRangeFactor = 1<<gyrRange;


}






/* SENSOR PWR */

void ICM20948_DMA::sleep(bool sleep) {
    uint8_t temp = readRegister8(0, static_cast<uint8_t>(ICM20948_Bank0_Registers::ICM20948_PWR_MGMT_1));
    if(sleep) {
        temp |= static_cast<uint8_t>(REGISTER_BITS::ICM20948_SLEEP);
    }else{
        temp &= ~static_cast<uint8_t>(REGISTER_BITS::ICM20948_SLEEP);
    }
    writeRegister8(0, static_cast<uint8_t>(ICM20948_Bank0_Registers::ICM20948_PWR_MGMT_1), temp);
}

void ICM20948_DMA::enableAcc(bool enAcc){
    uint8_t temp = readRegister8(0, static_cast<uint8_t>(ICM20948_Bank0_Registers::ICM20948_PWR_MGMT_2));
    if(enAcc){

        temp &= ~static_cast<uint8_t>(REGISTER_BITS::ICM20948_ACC_EN);

    }else{

    temp |= static_cast<uint8_t>(REGISTER_BITS::ICM20948_ACC_EN);
    }

    writeRegister8(0,static_cast<uint8_t>(ICM20948_Bank0_Registers::ICM20948_PWR_MGMT_2),temp);
}

void ICM20948_DMA::enableGyr(bool enGyr){
    uint8_t temp = readRegister8(0, static_cast<uint8_t>(ICM20948_Bank0_Registers::ICM20948_PWR_MGMT_2));
    if(enGyr){

        temp &= ~static_cast<uint8_t>(REGISTER_BITS::ICM20948_GYR_EN);

    }else{

        temp |= static_cast<uint8_t>(REGISTER_BITS::ICM20948_ACC_EN);
    }
}



/* SUPPORT */


void ICM20948_DMA::reset_ICM20948() {
    writeRegister8(0,static_cast<uint8_t>(ICM20948_Bank0_Registers::ICM20948_PWR_MGMT_1),
                   static_cast<uint8_t>(REGISTER_BITS::ICM20948_RESET));
    delay(100); 
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

/* magneto meter */
/* magneto meter */
bool ICM20948_DMA::init_AK09916() {
    bool succes = false;
    reset_ICM20948();
    enableI2CMaster();
    reset_AK09916();
    sleep(false);

    writeRegister8(2, static_cast<uint8_t>(ICM20948_Bank_2_Registers::ODR_ALIGN_EN), 1);

    uint8_t trying = 0;
    while (!succes && trying < 10) {
        delay(10);
        enableI2CMaster();
        delay(10);

        uint16_t who = whoAmI_AK09916();
        if (!((who == AK09916_WHO_AM_I_1) || (who == AK09916_WHO_AM_I_2))) {
            resetI2CMaster();
            succes = false;
            trying++;
        } else {
            succes = true;
            Serial.print("WHO_AM_I = 0x");
            Serial.println(who, HEX);
            break;
        }
    }

    if (succes) {
        setMagMode(AK09916_CONT_MODE_100HZ);
    }

    return succes;
}

void ICM20948_DMA::reset_AK09916() {
    AK09916_writeRegister8(static_cast<uint8_t>(AK09916_Registers::AK09916_CNTL_3), 0x01);
    delay(100);
}

uint16_t ICM20948_DMA::whoAmI_AK09916() {
    uint8_t ADDH_byte = AK09916_readRegister8(static_cast<uint8_t>(AK09916_Registers::AK09916_WIA_1));
    uint8_t ADDL_byte = AK09916_readRegister8(static_cast<uint8_t>(AK09916_Registers::AK09916_WIA_2));
    return (ADDH_byte << 8) | ADDL_byte;
}

void ICM20948_DMA::setMagMode(AK09916_opMode mode) {
    AK09916_writeRegister8(static_cast<uint8_t>(AK09916_Registers::AK09916_CNTL_2), mode);
    if (mode != AK09916_PWR_DOWN) {
        AK09916_enableMagRead(static_cast<uint8_t>(AK09916_Registers::AK09916_HXL), 0x08);
    }
}

/* I2C MASTER */
void ICM20948_DMA::enableI2CMaster() {
    writeRegister8(0, static_cast<uint8_t>(ICM20948_Bank0_Registers::ICM20948_USER_CTRL),
                   static_cast<uint8_t>(REGISTER_BITS::ICM20948_I2C_MST_EN));
    writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_MST_CTRL), 0x50);
}

void ICM20948_DMA::resetI2CMaster() {
    uint8_t temp = readRegister8(0, static_cast<uint8_t>(ICM20948_Bank0_Registers::ICM20948_USER_CTRL));
    temp |= ICM20948_I2C_MST_RST;
    writeRegister8(0, static_cast<uint8_t>(ICM20948_Bank0_Registers::ICM20948_USER_CTRL), temp);
}

/* I2C READ WRITE FOR AK09916 USING SLV0 */
uint8_t ICM20948_DMA::AK09916_readRegister8(uint8_t reg) {
    writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV0_ADDR),
                   static_cast<uint8_t>(ICM20948_CONSTANTS::AK09916_ADDRESS) | 0x80); // read
    writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV0_REG), reg);
    writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV0_CTRL),
                   static_cast<uint8_t>(REGISTER_BITS::ICM20948_I2C_SLVX_EN) | 0x01);

    // wait for transaction complete
    delay(10);
    unsigned long startMillis = millis();
    while (true) {
        uint8_t status = readRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV0_CTRL));
        if ((status & 0x80) == 0) break;
        if ((millis() - startMillis) > 100) {
            resetI2CMaster();
            break;
        }
        delay(1);
    }

    return readRegister8(0, static_cast<uint8_t>(ICM20948_Bank0_Registers::ICM20948_EXT_SLV_SENS_DATA_00));
}

void ICM20948_DMA::AK09916_writeRegister8(uint8_t reg, uint8_t val) {
    writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV0_ADDR),
                   static_cast<uint8_t>(ICM20948_CONSTANTS::AK09916_ADDRESS)); // write
    writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV0_DO), val);
    writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV0_REG), reg);
    writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV0_CTRL),
                   static_cast<uint8_t>(REGISTER_BITS::ICM20948_I2C_SLVX_EN) | 0x01);

    // wait for transaction complete
    delay(10);
    unsigned long startMillis = millis();
    while (true) {
        uint8_t status = readRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV0_CTRL));
        if ((status & 0x80) == 0) break;
        if ((millis() - startMillis) > 100) {
            resetI2CMaster();
            break;
        }
        delay(1);
    }
}

/* mag data read enable using SLV0 */
void ICM20948_DMA::AK09916_enableMagRead(uint8_t reg, uint8_t byte) {
    writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV0_ADDR),
                   static_cast<uint8_t>(ICM20948_CONSTANTS::AK09916_ADDRESS) | 0x80); // read
    writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV0_REG), reg);
    writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV0_CTRL),
                   static_cast<uint8_t>(REGISTER_BITS::ICM20948_I2C_SLVX_EN) | byte);

    // wait for transaction complete
    delay(10);
    unsigned long startMillis = millis();
    while (true) {
        uint8_t status = readRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV0_CTRL));
        if ((status & 0x80) == 0) break;
        if ((millis() - startMillis) > 100) {
            resetI2CMaster();
            break;
        }
        delay(1);
    }
}




