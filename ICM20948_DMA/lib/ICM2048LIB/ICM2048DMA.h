#ifndef ICM_2048

#define ICM_2048
#include <Arduino.h>  
#include "xyzFloat.h"
#include "ESP32DMASPIMaster.h"




enum class ICM20948_CYCLE : uint8_t{
    ICM20948_NO_CYCLE              = 0x00,
    ICM20948_GYR_CYCLE             = 0x10, 
    ICM20948_ACC_CYCLE             = 0x20,
    ICM20948_ACC_GYR_CYCLE         = 0x30,
    ICM20948_ACC_GYR_I2C_MST_CYCLE = 0x70
};


typedef enum ICM20948_INT_PIN_POL {
    ICM20948_ACT_HIGH, ICM20948_ACT_LOW
} ICM20948_intPinPol;

typedef enum MASKING_BIT{
    ICM20948_WRITE_MASKING_BIT = 0x7F,
    ICM20948_READ_MASKING_BIT  = 0x80,
    ICM20948_BANK_MASKING_BIT  = 0x03
}ICM20948_BitMasking;

enum class ICM20948_INT_TYPE : uint8_t{
    ICM20948_FSYNC_INT      = 0x01,
    ICM20948_WOM_INT        = 0x02,
    ICM20948_DMP_INT        = 0x04,
    ICM20948_DATA_READY_INT = 0x08,
    ICM20948_FIFO_OVF_INT   = 0x10,
    ICM20948_FIFO_WM_INT    = 0x20

};

enum class ICM20948_FIFO_TYPE :uint8_t{
    ICM20948_FIFO_ACC        = 0x10,
    ICM20948_FIFO_GYR        = 0x0E,
    ICM20948_FIFO_ACC_GYR    = 0x1E

};

typedef enum ICM20948_GYRO_RANGE {
    ICM20948_GYRO_RANGE_250, ICM20948_GYRO_RANGE_500, ICM20948_GYRO_RANGE_1000, ICM20948_GYRO_RANGE_2000
} ICM20948_gyroRange;
typedef enum ICM20948_DLPF {
    ICM20948_DLPF_0, ICM20948_DLPF_1, ICM20948_DLPF_2, ICM20948_DLPF_3, ICM20948_DLPF_4, ICM20948_DLPF_5, 
    ICM20948_DLPF_6, ICM20948_DLPF_7, ICM20948_DLPF_OFF
} ICM20948_dlpf;

typedef enum ICM20948_GYRO_AVG_LOW_PWR {
    ICM20948_GYR_AVG_1, ICM20948_GYR_AVG_2, ICM20948_GYR_AVG_4, ICM20948_GYR_AVG_8, ICM20948_GYR_AVG_16, 
    ICM20948_GYR_AVG_32, ICM20948_GYR_AVG_64, ICM20948_GYR_AVG_128
} ICM20948_gyroAvgLowPower;

typedef enum ICM20948_ACC_RANGE {
    ICM20948_ACC_RANGE_2G, ICM20948_ACC_RANGE_4G, ICM20948_ACC_RANGE_8G, ICM20948_ACC_RANGE_16G
} ICM20948_accRange;

typedef enum ICM20948_ACC_AVG_LOW_PWR {
    ICM20948_ACC_AVG_4, ICM20948_ACC_AVG_8, ICM20948_ACC_AVG_16, ICM20948_ACC_AVG_32
} ICM20948_accAvgLowPower;

typedef enum ICM20948_WOM_COMP {
    ICM20948_WOM_COMP_DISABLE, ICM20948_WOM_COMP_ENABLE
} ICM20948_womCompEn;

typedef enum AK09916_OP_MODE {
    AK09916_PWR_DOWN           = 0x00,
    AK09916_TRIGGER_MODE       = 0x01,
    AK09916_CONT_MODE_10HZ     = 0x02,
    AK09916_CONT_MODE_20HZ     = 0x04,
    AK09916_CONT_MODE_50HZ     = 0x06,
    AK09916_CONT_MODE_100HZ    = 0x08
} AK09916_opMode;

typedef enum ICM20948_ORIENTATION {
  ICM20948_FLAT, ICM20948_FLAT_1, ICM20948_XY, ICM20948_XY_1, ICM20948_YX, ICM20948_YX_1
} ICM20948_orientation;


//MASTER REGISTER



enum class ICM20948_CONSTANTS : uint8_t{
    AK09916_ADDRESS     = 0x0C
};

enum class ICM20948_Bank0_Registers : uint8_t {
    ICM20948_WHO_AM_I            = 0x00,
    ICM20948_USER_CTRL           = 0x03,
    ICM20948_LP_CONFIG           = 0x05,
    ICM20948_PWR_MGMT_1          = 0x06,
    ICM20948_PWR_MGMT_2          = 0x07,
    ICM20948_INT_PIN_CFG         = 0x0F,
    ICM20948_INT_ENABLE          = 0x10,
    ICM20948_INT_ENABLE_1        = 0x11,
    ICM20948_INT_ENABLE_2        = 0x12,
    ICM20948_INT_ENABLE_3        = 0x13,
    ICM20948_I2C_MST_STATUS      = 0x17,
    ICM20948_INT_STATUS          = 0x19,
    ICM20948_INT_STATUS_1        = 0x1A,
    ICM20948_INT_STATUS_2        = 0x1B,
    ICM20948_INT_STATUS_3        = 0x1C,
    ICM20948_DELAY_TIME_H        = 0x28,
    ICM20948_DELAY_TIME_L        = 0x29,
    ICM20948_ACCEL_OUT           = 0x2D,  // accel data registers begin
    ICM20948_GYRO_OUT            = 0x33,  // gyro data registers begin
    ICM20948_TEMP_OUT            = 0x39,
    ICM20948_EXT_SLV_SENS_DATA_00= 0x3B,
    ICM20948_EXT_SLV_SENS_DATA_01= 0x3C,
    ICM20948_FIFO_EN_1           = 0x66,
    ICM20948_FIFO_EN_2           = 0x67,
    ICM20948_FIFO_RST            = 0x68,
    ICM20948_FIFO_MODE           = 0x69,
    ICM20948_FIFO_COUNT          = 0x70,
    ICM20948_FIFO_R_W            = 0x72,
    ICM20948_DATA_RDY_STATUS     = 0x74,
    ICM20948_FIFO_CFG            = 0x76
};


enum class ICM20948_Bank_1_Registers : uint8_t {
    SELF_TEST_X_GYRO    = 0x02,
    SELF_TEST_Y_GYRO    = 0x03,
    SELF_TEST_Z_GYRO    = 0x04,
    SELF_TEST_X_ACCEL   = 0x0E,
    SELF_TEST_Y_ACCEL   = 0x0F,
    SELF_TEST_Z_ACCEL   = 0x10,
    XA_OFFS_H           = 0x14,
    XA_OFFS_L           = 0x15,
    YA_OFFS_H           = 0x17,
    YA_OFFS_L           = 0x18,
    ZA_OFFS_H           = 0x1A,
    ZA_OFFS_L           = 0x1B,
    TIMEBASE_CORR_PLL   = 0x28
};

enum class ICM20948_Bank_2_Registers : uint8_t {
    GYRO_SMPLRT_DIV     = 0x00,
    GYRO_CONFIG_1       = 0x01,
    GYRO_CONFIG_2       = 0x02,
    XG_OFFS_USRH        = 0x03,
    XG_OFFS_USRL        = 0x04,
    YG_OFFS_USRH        = 0x05,
    YG_OFFS_USRL        = 0x06,
    ZG_OFFS_USRH        = 0x07,
    ZG_OFFS_USRL        = 0x08,
    ODR_ALIGN_EN        = 0x09,
    ACCEL_SMPLRT_DIV_1  = 0x10,
    ACCEL_SMPLRT_DIV_2  = 0x11,
    ACCEL_INTEL_CTRL    = 0x12,
    ACCEL_WOM_THR       = 0x13,
    ACCEL_CONFIG        = 0x14,
    ACCEL_CONFIG_2      = 0x15,
    FSYNC_CONFIG        = 0x52,
    TEMP_CONFIG         = 0x53,
    MOD_CTRL_USR        = 0x54
};

enum class ICM20948_Bank3_Registers : uint8_t {
    I2C_MST_ODR_CFG     = 0x00,
    I2C_MST_CTRL        = 0x01,
    I2C_MST_DELAY_CTRL  = 0x02,
    I2C_SLV0_ADDR       = 0x03,
    I2C_SLV0_REG        = 0x04,
    I2C_SLV0_CTRL       = 0x05,
    I2C_SLV0_DO         = 0x06,
    I2C_SLV4_ADDR       = 0x13,
    I2C_SLV4_REG        = 0x14,
    I2C_SLV4_CTRL       = 0x15,
    I2C_SLV4_DO         = 0x16,
    I2C_SLV4_DI         = 0x17
};

enum class ICM20948_All_Bank : uint8_t{
    ICM20948_REG_BANK_SEL = 0x7F
};

enum class AK09916_Registers : uint8_t {
    AK09916_WIA_1    = 0x00,  // Who I am, Company ID
    AK09916_WIA_2    = 0x01,  // Who I am, Device ID
    AK09916_STATUS_1 = 0x10,
    AK09916_HXL      = 0x11,
    AK09916_HXH      = 0x12,
    AK09916_HYL      = 0x13,
    AK09916_HYH      = 0x14,
    AK09916_HZL      = 0x15,
    AK09916_HZH      = 0x16,
    AK09916_STATUS_2 = 0x18,
    AK09916_CNTL_2   = 0x31,
    AK09916_CNTL_3   = 0x32
};

enum class REGISTER_BITS : uint8_t {
    // Power Management Bits
    ICM20948_RESET              = 0x80,
    ICM20948_SLEEP              = 0x40,
    ICM20948_LP_EN              = 0x20,
    
    // User Control Bits
    ICM20948_I2C_MST_EN         = 0x20,
    ICM20948_FIFO_EN            = 0x40,
    
    // Sensor Enable Bits
    ICM20948_GYR_EN             = 0x07,
    ICM20948_ACC_EN             = 0x38,
    
    // Interrupt Pin Configuration Bits
    ICM20948_INT1_ACTL          = 0x80,
    ICM20948_INT_1_LATCH_EN     = 0x20,
    ICM20948_ACTL_FSYNC         = 0x08,
    ICM20948_INT_ANYRD_2CLEAR   = 0x10,
    ICM20948_BYPASS_EN          = 0x02,
    
    // Sync and I2C Bits
    ICM20948_FSYNC_INT_MODE_EN  = 0x06,
    ICM20948_I2C_SLVX_EN        = 0x80,
    
    // Magnetometer AK09916 Bits
    AK09916_16_BIT              = 0x10,
    AK09916_OVF                 = 0x08,
    AK09916_READ                = 0x80
} ;

enum class OTHERS{
        // AK09916 WHO_AM_I Values (16-bit)
    AK09916_WHO_AM_I_1      = 0x4809,  // Company ID (0x48) + Device ID (0x09)
    AK09916_WHO_AM_I_2      = 0x0948,  // Alternate byte order
    
    // ICM20948 WHO_AM_I Value (8-bit)
    ICM20948_WHO_AM_I_CONTENT = 0xEA,
    
    // I2C Master Control Bit
    ICM20948_I2C_MST_RST      = 0x02
};

class ICM20948_DMA {
protected:
    // DMA SPI interface
    ESP32DMASPI::Master master;
    uint8_t *dma_tx_buf{nullptr};
    uint8_t *dma_rx_buf{nullptr};

    // Pin assignments
    int csPin;
    int sdaPin;
    int sclPin;
    int adoPin;
    
    // Bank and data buffer
    uint8_t currentBank{0};
    uint8_t buffer[20];

    // Offsets & scaling
    xyzFloat accOffsetVal{};
    xyzFloat gyrOffsetVal{};
    uint8_t accRangeFactor{1};
    uint8_t gyrRangeFactor{1};

    // Low-level DMA operations
    void switchBank(uint8_t newBank);
    void writeRegister8(uint8_t bank, uint8_t reg, uint8_t val);
    uint8_t readRegister8(uint8_t bank, uint8_t reg);
    void readAllData(uint8_t *data);
    void reset_ICM20948();
    void spiTransfer(size_t len);

public:
    // Constructor: specify all SPI pins and optional I2C pins
   ICM20948_DMA(int scl,int ado,int sda,int cs);
   ~ICM20948_DMA();

    // Initialization and configuration
    bool init();
    void setSPIClockSpeed(unsigned long clock);
    void enableAcc(bool enAcc);
    void setAccRange(ICM20948_accRange accRange);
    void enableGyr(bool enGyr);
    void setGyrRange(ICM20948_gyroRange range);
    void setAccDLPF(ICM20948_dlpf dlpf);
    void sleep(bool sleep);

    // Data acquisition
    void readSensor();
    void getAccRawValues(xyzFloat *accRawVal);
    void getGyrRawValues(xyzFloat *gyrRawVal);
    float getTemperature();

    //check
    uint8_t whoAmI();
};
#endif




