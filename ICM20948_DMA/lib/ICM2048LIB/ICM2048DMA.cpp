#include "ICM2048DMA.h"

ICM20948_DMA::ICM20948_DMA(int scl,int ado,int sda,int cs): sclPin(scl), adoPin(ado), sdaPin(sda), csPin(cs){
    //pinMode(csPin, OUTPUT);
    //digitalWrite(csPin, HIGH);
    master = new ESP32DMASPI::Master();
    master->begin(HSPI, sclPin, adoPin, sdaPin, csPin);
    dma_tx_buf = master->allocDMABuffer(32);
    dma_rx_buf = master->allocDMABuffer(32);

}

// Destructor
ICM20948_DMA::~ICM20948_DMA() {
    if (master) {
        master->end();   
        delete master;   
        master = nullptr;
    }
    if (dma_tx_buf) {
        heap_caps_free(dma_tx_buf);
        dma_tx_buf = nullptr;
    }
    if (dma_rx_buf) {
        heap_caps_free(dma_rx_buf);
        dma_rx_buf = nullptr;
    }
}
// Main init 

bool ICM20948_DMA::init() {


    master->setDataMode(SPI_MODE0);
    master->setFrequency(1000000);  
    master->setMaxTransferSize(32);
    master->setQueueSize(1);

    delay(200);

    reset_ICM20948();   
    delay(100);

    uint8_t tries = 0;
    uint8_t who = 0;
    while (tries < 5) {
        reset_ICM20948(); 
        who = whoAmI();
        if (who == static_cast<uint8_t>(OTHERS::ICM20948_WHO_AM_I_CONTENT)) break;
        delay(100);
        tries++;
    }

   

    if (who != static_cast<uint8_t>(OTHERS::ICM20948_WHO_AM_I_CONTENT)) {
        return false;
    }

    accRangeFactor = 1;
    gyrRangeFactor = 1;

    sleep(false);
    enableAcc(true);
    enableGyr(true); 
    writeRegister8(2, static_cast<uint8_t>(ICM20948_Bank_2_Registers::ODR_ALIGN_EN), 1);

    return true;
}
/* recycle */
bool ICM20948_DMA::recycle(){
    if (master) {
        master->end();  
        delete master;   
        master = nullptr;
    }
    if (dma_tx_buf) {
        heap_caps_free(dma_tx_buf);
        dma_tx_buf = nullptr;
    }
    if (dma_rx_buf) {
        heap_caps_free(dma_rx_buf);
        dma_rx_buf = nullptr;
    }
    master = new ESP32DMASPI::Master();
    master->begin(HSPI, sclPin, adoPin, sdaPin, csPin);
    dma_tx_buf = master->allocDMABuffer(32);
    dma_rx_buf = master->allocDMABuffer(32);
    return init();

}


/* READ AND WRITE MASTER */

void ICM20948_DMA::writeRegister8(uint8_t bank, uint8_t reg, uint8_t val) {
    switchBank(bank);

    dma_tx_buf[0] = reg & ICM20948_WRITE_MASKING_BIT ;
    dma_tx_buf[1] = val;

    spiTransfer(2);
    delayMicroseconds(5);
}

uint8_t ICM20948_DMA::readRegister8(uint8_t bank, uint8_t reg) {
    switchBank(bank);

    dma_tx_buf[0]= reg | ICM20948_READ_MASKING_BIT;
    dma_tx_buf[1] = 0x00;
    spiTransfer(2);


    delayMicroseconds(5);
    return dma_rx_buf[1];

}
/* sensor read */

void ICM20948_DMA::readSensor(){
    uint8_t Reg = static_cast<uint8_t>(ICM20948_Bank0_Registers::ICM20948_ACCEL_OUT)| ICM20948_READ_MASKING_BIT;
    dma_tx_buf[0] = Reg;
    //zero the tx
    int i = 1;
    while(i<=20){
        dma_tx_buf[i] = 0x00;
        i++;
    }

    spiTransfer(21);
    i = 0;
    while(i<20){
        buffer[i] = dma_rx_buf[i+1];
        i++;
    }



}

void ICM20948_DMA::getAccRawValues(xyzFloat *accRawVal){
    //data assembly
    accRawVal->x = static_cast<int16_t>(((buffer[0]) << 8) | buffer[1]);
    accRawVal->y = static_cast<int16_t>(((buffer[2]) << 8) | buffer[3]);
    accRawVal->z = static_cast<int16_t>(((buffer[4]) << 8) | buffer[5]);
}
 
void ICM20948_DMA::getGyrRawValues(xyzFloat *gyrRawVal){

    gyrRawVal->x = static_cast<int16_t>(((buffer[0]) << 8) | buffer[1]);
    gyrRawVal->y = static_cast<int16_t>(((buffer[2]) << 8) | buffer[3]);
    gyrRawVal->z = static_cast<int16_t>(((buffer[4]) << 8) | buffer[5]);
}

float ICM20948_DMA::getTemperature(){
    int16_t rawTemp = static_cast<int16_t>(((buffer[12]) << 8) | buffer[13]);
    return (rawTemp*1.0 - 0.0)/333.87 + 21.0f;
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

void ICM20948_DMA::spiTransfer(size_t len){
    size_t aligned_len = (len + 3) & ~0x03; // 4byte is missing SPI SLAVE
    master->queue(dma_tx_buf, dma_rx_buf, aligned_len);
    master->trigger();
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

        temp |= static_cast<uint8_t>(REGISTER_BITS::ICM20948_GYR_EN);
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

        dma_tx_buf[0] = static_cast<uint8_t>(ICM20948_All_Bank::ICM20948_REG_BANK_SEL) & ICM20948_WRITE_MASKING_BIT;
        dma_tx_buf[1] = ( newBank & ICM20948_BANK_MASKING_BIT ) << 4;

        spiTransfer(2);

        delayMicroseconds(10);
    }
}

/* magneto meter */
bool ICM20948_DMA::init_AK09916(){
    bool succes = false;
    reset_ICM20948();
    enableI2CMaster();
    reset_AK09916();
    sleep(false);
    writeRegister8(2, static_cast<uint8_t>(ICM20948_Bank_2_Registers::ODR_ALIGN_EN), 1);
    uint8_t trying = 0;
    while (!succes && trying < 10){
        delay(10);
        enableI2CMaster();
        delay(10);

        uint16_t who = whoAmI_AK09916();
        if(! ((who == AK09916_WHO_AM_I_1) || (who == AK09916_WHO_AM_I_2))){
            resetI2CMaster();
            succes = false;
            trying++;
            

        }else{
            succes = true;
            break;
        }
    }
    if(succes){
        //default mode
        setMagMode(AK09916_CONT_MODE_100HZ);

    }
    return succes;


}
void ICM20948_DMA::reset_AK09916(){
    AK09916_writeRegister8(static_cast<uint8_t>(AK09916_Registers::AK09916_CNTL_3 ),0x01);
    delay(100);
    
}

uint16_t ICM20948_DMA::whoAmI_AK09916(){
    uint8_t ADDH_byte = AK09916_readRegister8(static_cast<uint8_t>(AK09916_Registers::AK09916_WIA_1));
    uint8_t ADDL_byte = AK09916_readRegister8(static_cast<uint8_t>(AK09916_Registers::AK09916_WIA_2));
    return (ADDH_byte<<8) | ADDL_byte;

}
void ICM20948_DMA::setMagMode(AK09916_opMode mode){
    AK09916_writeRegister8(static_cast<uint8_t>(AK09916_Registers::AK09916_CNTL_2 ),mode);
    if(mode != AK09916_PWR_DOWN){
        AK09916_enableMagRead(static_cast<uint8_t>(AK09916_Registers::AK09916_HXL ),0x08);

    }

}

/* I2C MASTER */

void ICM20948_DMA::enableI2CMaster(){
    writeRegister8(0,static_cast<uint8_t>(ICM20948_Bank0_Registers::ICM20948_USER_CTRL),
    static_cast<uint8_t>(REGISTER_BITS::ICM20948_I2C_MST_EN));
    writeRegister8(3,static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_MST_CTRL),0x05);//set clock I2C
}

void ICM20948_DMA::resetI2CMaster(){
    uint8_t temp = readRegister8(0,static_cast<uint8_t>(ICM20948_Bank0_Registers::ICM20948_USER_CTRL));
    temp |= ICM20948_I2C_MST_RST ;
    writeRegister8(0,static_cast<uint8_t>(ICM20948_Bank0_Registers::ICM20948_USER_CTRL),temp);
    
}

/* I2C READ WRITE FOR AK09916 */
uint8_t ICM20948_DMA::AK09916_readRegister8(uint8_t reg){
    writeRegister8(3,static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_ADDR),
    static_cast<uint8_t>(ICM20948_CONSTANTS::AK09916_ADDRESS) | static_cast<uint8_t>(REGISTER_BITS::AK09916_READ));
    writeRegister8(3,static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_REG ),reg);//tell register read
    writeRegister8(3,static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_CTRL),static_cast<uint8_t>(REGISTER_BITS::ICM20948_I2C_SLVX_EN));

    //waiting register to clear and avoid run away code
    unsigned long int startMillis = millis(); //avoid overflow
    while ((readRegister8(3,static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_CTRL)) & static_cast<uint8_t>(REGISTER_BITS::ICM20948_I2C_SLVX_EN))
    && (millis() - startMillis < 100));
    return readRegister8(3,static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_DI));

}

void ICM20948_DMA::AK09916_writeRegister8(uint8_t reg , uint8_t val){
    writeRegister8(3,static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_ADDR),static_cast<uint8_t>(ICM20948_CONSTANTS::AK09916_ADDRESS));
    writeRegister8(3,static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_DO),val);
    writeRegister8(3,static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_REG ),reg); //tell register write 
    writeRegister8(3,static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_CTRL),static_cast<uint8_t>(REGISTER_BITS::ICM20948_I2C_SLVX_EN));

    //waiting register to clear and avoid run away code
    unsigned long int startMillis = millis(); //avoid overflow
    while ((readRegister8(3,static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_CTRL)) & static_cast<uint8_t>(REGISTER_BITS::ICM20948_I2C_SLVX_EN))
    && (millis() - startMillis < 100));

}

/*mag data read enable*/
void ICM20948_DMA::AK09916_enableMagRead(uint8_t reg ,uint8_t byte){
    writeRegister8(3,static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_ADDR),
    static_cast<uint8_t>(ICM20948_CONSTANTS::AK09916_ADDRESS) | static_cast<uint8_t>(REGISTER_BITS::AK09916_READ));
    writeRegister8(3,static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_REG ),reg);
    writeRegister8(3,static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_CTRL),static_cast<uint8_t>(REGISTER_BITS::ICM20948_I2C_SLVX_EN) | byte);
    delay(12);

}
