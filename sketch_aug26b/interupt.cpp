#include "interupt.h"

// Initialize static member variables outside the class definition
unsigned int interupt::debauch_t = 0;
unsigned long interupt::last_millis = 0;
TaskHandle_t interupt::task3 = NULL;

// Constructor implementation
interupt::interupt() {
  // This constructor is defined, but does not perform any setup.
}

// Interrupt handler implementation
void IRAM_ATTR interupt::interupts() {
  unsigned long _current_millis = millis();
  if (_current_millis - interupt::last_millis < interupt::debauch_t) {
    return;
  }
  xTaskNotifyGive(interupt::task3);
  interupt::last_millis = _current_millis;
  Serial.println("INTERUPTING ALL TASK");
}

// Begin function implementation
int interupt::begin(int button, TaskHandle_t task3, unsigned int debauch_t) {
  // Assign member variables
  interupt::debauch_t = debauch_t;
  interupt::task3 = task3;

  // Set up the interrupt
  pinMode(button, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(button), interupt::interupts, FALLING);

  return 0;
}