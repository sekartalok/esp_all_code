#include "ESP32SERVO.h"

 ESP32SERVO::ESP32SERVO(uint8_t pinPwm): pinPwm(pinPwm){
    timer0 = NULL;
    timerMux0 = portMUX_INITIALIZER_UNLOCKED;
    duty = 0;
    instance = this;
 }

 void IRAM_ATTR ESP32SERVO::TimerHandler0(){
    if (instance == nullptr) return;

    portENTER_CRITICAL_ISR(&instance->timerMux0);
    ledcWrite(ESP32SERVO_ledcChannel, instance->duty);
    portEXIT_CRITICAL_ISR(&instance->timerMux0);

 }
 void ESP32SERVO::begin(unsigned int minUs = 700 , unsigned int maxUs = 2300){


  this->minUs = minUs;
  this->maxUs = maxUs; 

  ledcSetup(ESP32SERVO_ledcChannel , ESP32SERVO_frequency , ESP32SERVO_resolution);
  ledcAttachPin(pinPwm, ESP32SERVO_ledcChannel);
  ledcWrite(ESP32SERVO_ledcChannel, 0);

  timer0 = timerBegin(0, 80, true);
  timerAttachInterrupt(timer0, &TimerHandler0, false);
  timerAlarmWrite(timer0, 20000, true); // 20 ms (50 Hz)
  timerAlarmEnable(timer0);

 }

 void ESP32SERVO::setServoTo(uint8_t degree){
    
    portENTER_CRITICAL(&timerMux0);
    duty = angleToDuty(degree);
    portEXIT_CRITICAL(&timerMux0);

 }

 void ESP32SERVO::rotationTest(){
    smoothMove(0,180);
    delay(5);
    smoothMove(180,0);
    

 }

 void ESP32SERVO::smoothMove(int fromAngle, int toAngle, float step = 0.5f, int delayMs = 10) {
  if (fromAngle < toAngle) {
    for (float angle = fromAngle; angle <= toAngle; angle += step) {
      portENTER_CRITICAL(&timerMux0);
      duty = angleToDuty((int)angle);
      portEXIT_CRITICAL(&timerMux0);
      delay(delayMs);
    }
  } else {
    for (float angle = fromAngle; angle >= toAngle; angle -= step) {
      portENTER_CRITICAL(&timerMux0);
      duty = angleToDuty((int)angle);
      portEXIT_CRITICAL(&timerMux0);
      delay(delayMs);
    }
  }
}



 