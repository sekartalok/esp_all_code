#ifndef ESP32SERVO_H
#define ESP32SERVO_H

#include <Arduino.h>

typedef enum {
    ESP32SERVO_maxServo   = 16,
    ESP32SERVO_servoGroup = 4,
    ESP32SERVO_timerGroup = 4
} groupSetting;

typedef enum {
    ESP32SERVO_frequency  = 50,
    ESP32SERVO_resolution = 12
} generalSetting;

class ESP32SERVO {
public:
    explicit ESP32SERVO(uint8_t pinPwm);
    void begin(unsigned int minUs = 500, unsigned int maxUs = 2400);
    void setServoTo(uint8_t degree);
    void smoothMove(int fromAngle, int toAngle, float step = 0.5f, int delayMs = 10);
    void rotationTest();

private:
    uint8_t pinPwm;
    uint8_t ledcChannel;
    uint8_t groupId;
    volatile uint32_t duty;
    unsigned int minUs, maxUs;

    static ESP32SERVO* instances[ESP32SERVO_maxServo];
    static uint8_t servoCount;

    static hw_timer_t* timers[ESP32SERVO_timerGroup];
    static portMUX_TYPE timerMuxes[ESP32SERVO_timerGroup];

    // group handler array and functions
    static void (*groupHandlers[ESP32SERVO_timerGroup])();
    static void IRAM_ATTR updateGroup(uint8_t groupId);

    static void IRAM_ATTR onTimerGroup0();
    static void IRAM_ATTR onTimerGroup1();
    static void IRAM_ATTR onTimerGroup2();
    static void IRAM_ATTR onTimerGroup3();

    uint32_t angleToDuty(int angle);
};

#endif // ESP32SERVO_H
