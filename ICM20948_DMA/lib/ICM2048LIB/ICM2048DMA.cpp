#include "ICM2048DMA.h"

ICM20948_DMA::ICM20948_DMA(int scl,int ado,int sda,int cs): sclPin(scl), adoPin(ado), sdaPin(sda), csPin(cs){
    //pinMode(csPin, OUTPUT);
    //digitalWrite(csPin, HIGH);
    master.begin(HSPI, sclPin, adoPin, sdaPin, csPin);
    dma_tx_buf = master.allocDMABuffer(32);
    dma_rx_buf = master.allocDMABuffer(32);

}

// Destructor
ICM20948_DMA::~ICM20948_DMA() {
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


    master.setDataMode(SPI_MODE0);
    master.setFrequency(1000000);  
    master.setMaxTransferSize(32);
    master.setQueueSize(1);

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
    master.queue(dma_tx_buf, dma_rx_buf, aligned_len);
    master.trigger();
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

        dma_tx_buf[0] = static_cast<uint8_t>(ICM20948_All_Bank::ICM20948_REG_BANK_SEL) & ICM20948_WRITE_MASKING_BIT;
        dma_tx_buf[1] = ( newBank & ICM20948_BANK_MASKING_BIT ) << 4;

        spiTransfer(2);

        delayMicroseconds(10);
    }
}