#include "sd_integrate.h"
SemaphoreHandle_t SD_INTEGRATE::sd_card_gate;

SD_INTEGRATE::SD_INTEGRATE(){}
void SD_INTEGRATE::read_sd(){
    if(xSemaphoreTake(sd_card_gate,portMAX_DELAY) == pdTRUE){
        File file = SD.open("/log/mylog.txt");
        if(!file){
            xSemaphoreGive(sd_card_gate);
            return;
            
        }
        Serial.println(file.readString().c_str());
        file.close();
        xSemaphoreGive(sd_card_gate);
    }
}
void SD_INTEGRATE::send_sd(std::string data){
    if(xSemaphoreTake(sd_card_gate,portMAX_DELAY) == pdTRUE){
        if(!SD.exists("/log")){
            SD.mkdir("/log");
        }
        File file = SD.open("/log/mylog.txt","a");
        if(!file){
            xSemaphoreGive(sd_card_gate);
            return;
        }
        file.println(data.c_str());
        file.close();
        xSemaphoreGive(sd_card_gate);
        
    }
}

void SD_INTEGRATE::begin(){
    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
    SD.begin(SD_CS);
    sd_card_gate = xSemaphoreCreateBinary();
    xSemaphoreGive(sd_card_gate);
    
}
