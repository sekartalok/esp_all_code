#pragma once
#ifndef ESP32_ESCLIB
#include <Arduino.h>


typedef enum{
    ESP32_ESC_HZ  = 50,
    ESP32_ESC_RES = 12

}systemSetting;



class ESP32_ESC{
    public:

    explicit ESP32_ESC(unsigned int min , unsigned int max);
    void begin(unsigned int rotor0,unsigned int rotor1,unsigned int rotor2,unsigned int rotor3);
    void setRotor0Speed(uint8_t rotorSpeed);
    void setRotor1Speed(uint8_t rotorSpeed);
    void setRotor2Speed(uint8_t rotorSpeed);
    void setRotor3Speed(uint8_t rotorSpeed);




    private:

    //rotor

    typedef struct {
    unsigned int rotor0;
    unsigned int rotor1;
    unsigned int rotor2;
    unsigned int rotor3;

    }pinRotor;

    pinRotor rotorEncapsule;

    static volatile uint32_t duty0;
    static volatile uint32_t duty1;
    static volatile uint32_t duty2;
    static volatile uint32_t duty3;

    typedef enum{
        ESP32_ESC_CHANNEL_ROTOR_0,ESP32_ESC_CHANNEL_ROTOR_1,ESP32_ESC_CHANNEL_ROTOR_2,ESP32_ESC_CHANNEL_ROTOR_3

    }Channel;

    unsigned int minRotor;
    unsigned int maxRotor;

    //timer
    static hw_timer_t * timer0;
    static portMUX_TYPE portMUX0;

    static void IRAM_ATTR timerHandler0();

    uint32_t angleToDuty(int angle);

    

};





#endif