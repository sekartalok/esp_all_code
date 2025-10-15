#include "ICM2048DMA.h"

ICM20948_DMA::ICM20948_DMA(int scl,int ado,int sda,int cs): sclPin(scl), adoPin(ado), sdaPin(sda), csPin(cs){
    pinMode(csPin, OUTPUT);
    digitalWrite(csPin, HIGH);

    SPI = new SPIClass(1);
}

// Destructor
ICM20948_DMA::~ICM20948_DMA() {

    if(SPI){
        SPI->end();
        delete SPI;
        SPI = nullptr;

    }
    if (DMASPI) {
        DMASPI->end();   
        delete DMASPI;   
        DMASPI = nullptr;
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

/* ========================= ICM20948 GENERAL SETUP ========================= */
void ICM20948_DMA::dmaEnable(){

    delay(50);
    SPI->end();
    delete SPI;
    SPI =nullptr;
    delay(10);


    DMACON = true;

    digitalWrite(csPin, HIGH);
    DMASPI = new ESP32DMASPI::Master();
    DMASPI->begin(HSPI, sclPin, adoPin, sdaPin, csPin);
    dma_tx_buf = DMASPI->allocDMABuffer(400);
    dma_rx_buf = DMASPI->allocDMABuffer(400);

    delay(10);

    DMASPI->setDataMode(SPI_MODE0);
    DMASPI->setFrequency(1000000);  
    DMASPI->setMaxTransferSize(400);
    DMASPI->setQueueSize(1);

    delay(100);





}

bool ICM20948_DMA::init() {

    SPI->begin(sclPin, adoPin, sdaPin, csPin);
    spi_setting = SPISettings(1000000, MSBFIRST, SPI_MODE0);

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
    reset_ICM20948();
    delay(50);

    return true;
}
void ICM20948_DMA::end(){
    spi_setting = SPISettings();
    if(SPI){
        SPI->end();
        delete SPI;
        SPI = nullptr;

    }
    if (DMASPI) {
        DMASPI->end();   
        delete DMASPI;   
        DMASPI = nullptr;
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

bool ICM20948_DMA::allInit(){
    if(!init()){
        return false;
    }
    if(!init_AK09916()){
        end();
        return false;
    }
    delay(50);
    setAccRange(ICM20948_ACC_RANGE_2G);
    setAccDLPF(ICM20948_DLPF_6);
    delay(10);
    setGyrRange(ICM20948_GYRO_RANGE_250);
    setGyrDLPF(ICM20948_DLPF_6);
    delay(10);
    setMagMode(AK09916_CONT_MODE_20HZ);
    delay(50);
    

    dmaEnable();
    delay(50);
    return true;
}
/* recycle */

/* INOP */
/*
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

}*/



/* ========================= READ AND WRRITE MASTER FOR 8BIT REG ========================= */

void ICM20948_DMA::writeRegister8(uint8_t bank, uint8_t reg, uint8_t val) {
    switchBank(bank);
    if(DMACON){
        dma_tx_buf[0] = reg & ICM20948_WRITE_MASKING_BIT ;
        dma_tx_buf[1] = val;

    spiTransfer(2);
    }else{
        digitalWrite(csPin, LOW);
        SPI->beginTransaction(spi_setting);
    
        SPI->transfer(reg & ICM20948_WRITE_MASKING_BIT); // write mask
        SPI->transfer(val);

        SPI->endTransaction();
        digitalWrite(csPin, HIGH);

    }
    delayMicroseconds(5);
    
}

uint8_t ICM20948_DMA::readRegister8(uint8_t bank, uint8_t reg) {
    switchBank(bank);
    uint8_t Return;
    if(DMACON){
    dma_tx_buf[0]= reg | ICM20948_READ_MASKING_BIT;
    dma_tx_buf[1] = 0x00;
    spiTransfer(2);
    Return = dma_rx_buf[1];
    }else{
        digitalWrite(csPin, LOW);
        SPI->beginTransaction(spi_setting);

        SPI->transfer(reg | ICM20948_READ_MASKING_BIT); // read mask
        Return = SPI->transfer(0x00);

        SPI->endTransaction();
        digitalWrite(csPin, HIGH);
    }

    delayMicroseconds(5);
    return Return;

}

/* ========================= SENSOR DATA READ ========================= */

void ICM20948_DMA::readSensor(){
    switchBank(0);
    uint8_t Reg = static_cast<uint8_t>(ICM20948_Bank0_Registers::ICM20948_ACCEL_OUT)| ICM20948_READ_MASKING_BIT;
    if(DMACON){
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
    }else{
        digitalWrite(csPin,LOW);
        SPI->beginTransaction(spi_setting);
        SPI->transfer(Reg);
        int i = 0;
        while(i<20){
            buffer[i] = SPI->transfer(0x00);
            i++;
        }
        SPI->endTransaction();
        digitalWrite(csPin,HIGH);
        
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

void ICM20948_DMA::getMagValues(xyzFloat *mag) {
    mag->x = static_cast<int16_t>((buffer[15]) << 8) | buffer[14];
    mag->y = static_cast<int16_t>((buffer[17]) << 8) | buffer[16];
    mag->z = static_cast<int16_t>((buffer[19]) << 8) | buffer[18];



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

void ICM20948_DMA::setGyrDLPF(ICM20948_dlpf dlpf){
    uint8_t temp = readRegister8(2,static_cast<uint8_t>(ICM20948_Bank_2_Registers::GYRO_CONFIG_1));
    if(dlpf!=ICM20948_DLPF_OFF){
        temp |= 0x01;
        temp &= 0xC7;
        temp |= (dlpf<<3);
    }else{
        temp &= 0xFE;
    }
 
    writeRegister8(2,static_cast<uint8_t>(ICM20948_Bank_2_Registers::GYRO_CONFIG_1),temp);


}


void ICM20948_DMA::spiTransfer(size_t len){
    size_t aligned_len = (len + 3) & ~0x03; // 4byte is missing SPI SLAVE


    DMASPI->queue(dma_tx_buf, dma_rx_buf, aligned_len);
    DMASPI->trigger();
}


/* ========================= ICM20948 SENSOR SET RANGE ========================= */


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




/* ========================= ICM20948 SENSOR PWR OPTIONS ========================= */

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




/* ========================= ICM20948 SUPPORT ========================= */


void ICM20948_DMA::reset_ICM20948() {
    writeRegister8(0,static_cast<uint8_t>(ICM20948_Bank0_Registers::ICM20948_PWR_MGMT_1),
                   static_cast<uint8_t>(REGISTER_BITS::ICM20948_RESET));
    delay(100); 
}
void ICM20948_DMA::switchBank(uint8_t newBank) {
    if (newBank != currentBank) {
        if(DMACON){
            currentBank = newBank;

            dma_tx_buf[0] = static_cast<uint8_t>(ICM20948_All_Bank::ICM20948_REG_BANK_SEL) & ICM20948_WRITE_MASKING_BIT;
            dma_tx_buf[1] = ( newBank & ICM20948_BANK_MASKING_BIT ) << 4;

            spiTransfer(2);
        }else{
            currentBank = newBank;
            digitalWrite(csPin, LOW);
            SPI->beginTransaction(spi_setting);
        

            SPI->transfer(static_cast<uint8_t>(ICM20948_All_Bank::ICM20948_REG_BANK_SEL) & ICM20948_WRITE_MASKING_BIT);
            SPI->transfer((newBank & ICM20948_BANK_MASKING_BIT ) << 4);

        
            SPI->endTransaction();
            digitalWrite(csPin, HIGH);

        }

        delayMicroseconds(10);
    }
}




/* ========================= AK09916 MAGNETOMETER ========================= */

bool ICM20948_DMA::init_AK09916() {
    bool success = false;


    reset_ICM20948();
    delay(100);

    enableI2CMaster();
    delay(10);

    reset_AK09916();
    delay(100);

    sleep(false);
    writeRegister8(2, static_cast<uint8_t>(ICM20948_Bank_2_Registers::ODR_ALIGN_EN), 1);

    uint8_t tries = 0;
    while (!success && tries < 5) {
        delay(10);
        enableI2CMaster();
        delay(10);

        uint16_t who = whoAmI_AK09916();
 

        if ((who == AK09916_WHO_AM_I_1) || (who == AK09916_WHO_AM_I_2)) {
            success = true;
            break;
        } else {

            resetI2CMaster();
            tries++;
        }
    }

    if (success) {

        setMagMode(AK09916_CONT_MODE_100HZ);
    } 


    return success;
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
    delay(10);

    if (mode != AK09916_PWR_DOWN) {
        switchBank(3);

        writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV0_ADDR),
                       static_cast<uint8_t>(ICM20948_CONSTANTS::AK09916_ADDRESS) | 0x80); // Read mode
        writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV0_REG),
                       static_cast<uint8_t>(AK09916_Registers::AK09916_HXL)); // Start from HXL
        writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV0_CTRL), 0x88); // Enable + 8 bytes
   
    }
}

/* ========================= I2C MASTER CONTROL ========================= */

void ICM20948_DMA::enableI2CMaster() {
    switchBank(0);
    writeRegister8(0, static_cast<uint8_t>(ICM20948_Bank0_Registers::ICM20948_USER_CTRL),
                   static_cast<uint8_t>(REGISTER_BITS::ICM20948_I2C_MST_EN));
    switchBank(3);
    writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_MST_CTRL), 0x07); // 400 kHz I2C clock
}

void ICM20948_DMA::resetI2CMaster() {
    uint8_t temp = readRegister8(0, static_cast<uint8_t>(ICM20948_Bank0_Registers::ICM20948_USER_CTRL));
    temp |= ICM20948_I2C_MST_RST;
    writeRegister8(0, static_cast<uint8_t>(ICM20948_Bank0_Registers::ICM20948_USER_CTRL), temp);
    delay(10);
}

/* ========================= AK09916 REGISTER ACCESS ========================= */

uint8_t ICM20948_DMA::AK09916_readRegister8(uint8_t reg) {
    switchBank(3);

    writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_ADDR),
              static_cast<uint8_t>(ICM20948_CONSTANTS::AK09916_ADDRESS) | static_cast<uint8_t>(REGISTER_BITS::AK09916_READ)); // Read mode
    writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_REG), reg);
    writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_CTRL), static_cast<uint8_t>(REGISTER_BITS::ICM20948_I2C_SLVX_EN));

    delay(10);
    uint32_t startmilis = millis();
    //avoid code hang
    while(readRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_CTRL) & static_cast<uint8_t>(REGISTER_BITS::ICM20948_I2C_SLVX_EN) )){
        if(millis() - startmilis < 100){
            break;
        }
        
    }
    return readRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_DI));

}

void ICM20948_DMA::AK09916_writeRegister8(uint8_t reg, uint8_t val) {
    switchBank(3);

    writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_ADDR),
              static_cast<uint8_t>(ICM20948_CONSTANTS::AK09916_ADDRESS)); // Write
    writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_DO), val);
    writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_REG), reg);
    writeRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_CTRL), static_cast<uint8_t>(REGISTER_BITS::ICM20948_I2C_SLVX_EN));

    delay(10);
    //avoid code hang
    uint32_t startmilis = millis();
    while(readRegister8(3, static_cast<uint8_t>(ICM20948_Bank3_Registers::I2C_SLV4_CTRL) & static_cast<uint8_t>(REGISTER_BITS::ICM20948_I2C_SLVX_EN) )){
        if(millis() - startmilis < 100){
            break;
        }
        
    }
}




