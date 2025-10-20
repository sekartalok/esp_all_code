#include "ESP32_ESC.h"


volatile uint32_t ESP32_ESC::duty0 = 0;
volatile uint32_t ESP32_ESC::duty1 = 0;
volatile uint32_t ESP32_ESC::duty2 = 0;
volatile uint32_t ESP32_ESC::duty3 = 0;

hw_timer_t * ESP32_ESC::timer0 = nullptr;
portMUX_TYPE ESP32_ESC:: portMUX0 = portMUX_INITIALIZER_UNLOCKED;

ESP32_ESC::ESP32_ESC(unsigned int min , unsigned int max):minRotor(min),maxRotor(max){}


void IRAM_ATTR ESP32_ESC::timerHandler0(){
    portENTER_CRITICAL_ISR(&portMUX0);
    ledcWrite(ESP32_ESC_CHANNEL_ROTOR_0,duty0);
    ledcWrite(ESP32_ESC_CHANNEL_ROTOR_1,duty1);
    ledcWrite(ESP32_ESC_CHANNEL_ROTOR_2,duty2);
    ledcWrite(ESP32_ESC_CHANNEL_ROTOR_3,duty3);
    portEXIT_CRITICAL_ISR(&portMUX0);
}



void ESP32_ESC::begin(unsigned int rotor0,unsigned int rotor1,unsigned int rotor2,unsigned int rotor3){
    rotorEncapsule.rotor0 = rotor0;
    rotorEncapsule.rotor1 = rotor1;
    rotorEncapsule.rotor2 = rotor2;
    rotorEncapsule.rotor3 = rotor3;

    //LED PWM PORT SET

    //rotor0
    ledcSetup(ESP32_ESC_CHANNEL_ROTOR_0,ESP32_ESC_HZ,ESP32_ESC_RES);
    ledcAttachPin(rotorEncapsule.rotor0,ESP32_ESC_CHANNEL_ROTOR_0);

    //rotor1
    ledcSetup(ESP32_ESC_CHANNEL_ROTOR_1,ESP32_ESC_HZ,ESP32_ESC_RES);
    ledcAttachPin(rotorEncapsule.rotor1,ESP32_ESC_CHANNEL_ROTOR_1);

    //rotor2
    ledcSetup(ESP32_ESC_CHANNEL_ROTOR_2,ESP32_ESC_HZ,ESP32_ESC_RES);
    ledcAttachPin(rotorEncapsule.rotor2,ESP32_ESC_CHANNEL_ROTOR_2);

    //rotor3
    ledcSetup(ESP32_ESC_CHANNEL_ROTOR_3,ESP32_ESC_HZ,ESP32_ESC_RES);
    ledcAttachPin(rotorEncapsule.rotor3,ESP32_ESC_CHANNEL_ROTOR_3);

    delay(20);

    //init timer0 and mutex

    timer0 = timerBegin(0,80,true);
    timerAttachInterrupt(timer0, &timerHandler0, false);
    timerAlarmWrite(timer0, 20000, true); // 20 ms (50 Hz)
    timerAlarmEnable(timer0);

}

//set angle of the freq
uint32_t ESP32_ESC::angleToDuty(int angle){
    int us = map(angle,0,180,minRotor,maxRotor);
    return (uint32_t)((us / 20000.0f) * 4095); //transfer to 12 bit
}

//set speed
void ESP32_ESC::setRotor0Speed(uint8_t rotorSpeed){
    portENTER_CRITICAL_ISR(&portMUX0);
    duty0 = angleToDuty(rotorSpeed);
    portEXIT_CRITICAL_ISR(&portMUX0);

}

void ESP32_ESC::setRotor1Speed(uint8_t rotorSpeed){
    portENTER_CRITICAL_ISR(&portMUX0);
    duty1 = angleToDuty(rotorSpeed);
    portEXIT_CRITICAL_ISR(&portMUX0);

}

void ESP32_ESC::setRotor2Speed(uint8_t rotorSpeed){
    portENTER_CRITICAL_ISR(&portMUX0);
    duty2 = angleToDuty(rotorSpeed);
    portEXIT_CRITICAL_ISR(&portMUX0);

}

void ESP32_ESC::setRotor3Speed(uint8_t rotorSpeed){
    portENTER_CRITICAL_ISR(&portMUX0);
    duty3 = angleToDuty(rotorSpeed);
    portEXIT_CRITICAL_ISR(&portMUX0);

}

