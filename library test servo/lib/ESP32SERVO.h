#pragma once
#ifndef ESP32SERVOLIB
#define ESP32SERVOLIB
#include <Arduino.h>

typedef enum {

    ESP32SERVO_ledcChannel = 1,
    ESP32SERVO_resolution  = 12,
    ESP32SERVO_frequency   = 50

}pwmSettingt;

class ESP32SERVO {
    private:

    unsigned int minUs;
    unsigned int maxUs;

    hw_timer_t *timer0;
    portMUX_TYPE timerMux0;
    volatile uint32_t duty{0};



    uint32_t angleToDuty(int angle);
    void IRAM_ATTR TimerHandler0();

    public:

    ESP32SERVO(uint8_t pinPwm);

    void begin(unsigned int minUs = 700 , unsigned int maxUs = 2300);
    void rotationTest();
    void setServoTo(uint8_t degree);

    

};






#endif