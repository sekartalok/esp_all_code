#include "ESP32SERVO.h"
#include "driver/ledc.h" // optional, but can be useful if you later call ESP-IDF functions

// Statics
ESP32SERVO* ESP32SERVO::instances[ESP32SERVO_maxServo] = { nullptr };
uint8_t ESP32SERVO::servoCount = 0;

hw_timer_t* ESP32SERVO::timers[ESP32SERVO_timerGroup] = { nullptr };
portMUX_TYPE ESP32SERVO::timerMuxes[ESP32SERVO_timerGroup] = {
    portMUX_INITIALIZER_UNLOCKED,
    portMUX_INITIALIZER_UNLOCKED,
    portMUX_INITIALIZER_UNLOCKED,
    portMUX_INITIALIZER_UNLOCKED
};

// forward-declare group handlers array (definition below)
void (*ESP32SERVO::groupHandlers[ESP32SERVO_timerGroup])() = {
    &ESP32SERVO::onTimerGroup0,
    &ESP32SERVO::onTimerGroup1,
    &ESP32SERVO::onTimerGroup2,
    &ESP32SERVO::onTimerGroup3
};

// Constructor
ESP32SERVO::ESP32SERVO(uint8_t pinPwm) : pinPwm(pinPwm), duty(0), minUs(700), maxUs(2300) {
    if (servoCount < ESP32SERVO_maxServo) {
        ledcChannel = servoCount;
        groupId = ledcChannel / ESP32SERVO_servoGroup;
        instances[servoCount] = this;
        servoCount++;
    }
}

// Begin: configure that object's LEDC channel and ensure group timer exists
void ESP32SERVO::begin(unsigned int minUs_in, unsigned int maxUs_in) {
    this->minUs = minUs_in;
    this->maxUs = maxUs_in;

    ledcSetup(ledcChannel, ESP32SERVO_frequency, ESP32SERVO_resolution);
    ledcAttachPin(pinPwm, ledcChannel);
    ledcWrite(ledcChannel, 0);

    // Create the group timer once (use groupId as timer number 0..3)
    if (timers[groupId] == nullptr) {
        timers[groupId] = timerBegin(groupId, 80, true); // timer number = groupId (0..3), prescaler=80 => 1us tick
        // attach the correct handler for this group
        timerAttachInterrupt(timers[groupId], groupHandlers[groupId], true);
        timerAlarmWrite(timers[groupId], 20000, true); // 20 ms = 50Hz
        timerAlarmEnable(timers[groupId]);
    }
}

// 4 group wrappers - IRAM_ATTR so callable from ISR
void IRAM_ATTR ESP32SERVO::onTimerGroup0() { updateGroup(0); }
void IRAM_ATTR ESP32SERVO::onTimerGroup1() { updateGroup(1); }
void IRAM_ATTR ESP32SERVO::onTimerGroup2() { updateGroup(2); }
void IRAM_ATTR ESP32SERVO::onTimerGroup3() { updateGroup(3); }

// Group updater called from ISR — IRAM_ATTR required
void IRAM_ATTR ESP32SERVO::updateGroup(uint8_t groupId) {
    int start = groupId * ESP32SERVO_servoGroup;
    int end = start + ESP32SERVO_servoGroup;

    // iterate the group's servo slot range
    for (int i = start; i < end; ++i) {
        ESP32SERVO* temp = instances[i];
        if (temp != nullptr) {
            // protect the instance duty read/write using the group mux
            portENTER_CRITICAL_ISR(&timerMuxes[groupId]);
            uint32_t localDuty = temp->duty;
            portEXIT_CRITICAL_ISR(&timerMuxes[groupId]);

            // write PWM duty for this channel. ledcWrite from ISR is used here;
            // many people do this successfully in Arduino core for ESP32.
            ledcWrite(temp->ledcChannel, localDuty);
        }
    }


}

// Set angle (updates duty atomically for instance's group)
void ESP32SERVO::setServoTo(uint8_t degree) {
    // find this object's group and protect its duty
    portENTER_CRITICAL(&timerMuxes[groupId]);
    duty = angleToDuty(degree);
    portEXIT_CRITICAL(&timerMuxes[groupId]);
}

// Smooth movement (calls setServoTo)
void ESP32SERVO::smoothMove(int fromAngle, int toAngle, float step, int delayMs) {
    if (fromAngle < toAngle) {
        for (float a = fromAngle; a <= toAngle; a += step) {
            setServoTo((int)a);
            delay(delayMs);
        }
    } else {
        for (float a = fromAngle; a >= toAngle; a -= step) {
            setServoTo((int)a);
            delay(delayMs);
        }
    }
}

void ESP32SERVO::rotationTest() {
    smoothMove(0, 180);
    delay(500);
    smoothMove(180, 0);
}

// Proper angle->duty: scale microseconds into LEDC resolution (0..(2^res -1))
uint32_t ESP32SERVO::angleToDuty(int angle) {
    int us = map(angle, 0, 180, minUs, maxUs);
    uint32_t maxDuty = (1u << ESP32SERVO_resolution) - 1u; // 4095 for 12-bit
    // (us / period_us) * maxDuty
    return (uint32_t)(( (float)us / 20000.0f ) * (float)maxDuty + 0.5f);
}
