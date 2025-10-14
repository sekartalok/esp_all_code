#pragma once
#ifndef ESP32_ESCLIB
#include <Arduino.h>

typedef enum {
    CHANNEL_0,CHANNEL_1,CHANNEL_2,CHANNEL_3
}channelControl;

typedef enum{
    ESP32_ESC_HZ  = 50,
    ESP32_ESC_RES = 12

}systemSetting;



class ESP32_ESC{
    public:

    explicit ESP32_ESC(unsigned int min , unsigned int max);
    void begin(unsigned int rotor0,unsigned int rotor1,unsigned int rotor2,unsigned int rotor3);
    




    private:

    typedef struct {
    unsigned int rotor0;
    unsigned int rotor1;
    unsigned int rotor2;
    unsigned int rotor3;

    }pinRotor;

};





#endif