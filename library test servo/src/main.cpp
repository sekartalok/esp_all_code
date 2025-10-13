#include "ESP32SERVO.h"


ESP32SERVO myServo1(12);
ESP32SERVO myServo5(15);
ESP32SERVO myServo3(18);
ESP32SERVO myServo4(38);
ESP32SERVO myServo2(16);

void setup() {
  myServo1.begin();
  myServo2.begin();
  myServo3.begin();
  myServo4.begin();
  myServo5.begin();
}

void loop() {
  myServo2.setServoTo(180);
  myServo4.setServoTo(180);
  delay(1000);
  myServo2.setServoTo(0);
  myServo4.setServoTo(0);
  delay(1000);
  myServo2.setServoTo(90);
  myServo4.setServoTo(90);
  delay(1000);
  myServo2.rotationTest();
  myServo4.rotationTest();

}

// put function definitions here:
