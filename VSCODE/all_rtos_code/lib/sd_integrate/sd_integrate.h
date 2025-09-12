#pragma once

#include <SD.h>
#include <SPI.h>
#include <FS.h>

#define SD_CS    13
#define SD_MOSI  11
#define SD_MISO  2
#define SD_SCLK  14

class SD_INTEGRATE{
    public:
    
    SD_INTEGRATE();

    void send_sd(std::string data);
    void read_sd();
    void begin();
    private:
    static SemaphoreHandle_t sd_card_gate;
};