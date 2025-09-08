#ifndef interupt_h
#define interupt_h

#include "Arduino.h"
#include "gpio.h"

// define abstractions 
class interupt {
public:
  // Constructor
  interupt();

  // main functions
  static void IRAM_ATTR interupts();

  int begin(TaskHandle_t task3, unsigned int debauch_t);

private:
  static unsigned int debauch_t;
  static unsigned long last_millis;
  static TaskHandle_t task3;
};

#endif