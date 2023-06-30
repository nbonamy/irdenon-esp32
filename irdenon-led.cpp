#include <Arduino.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

void initLed() {
  pinMode(LED_BUILTIN, OUTPUT);
}

void ledOn() {
  //Serial.println("[LED ] ON");
  digitalWrite(LED_BUILTIN, HIGH);
}

void ledOff() {
  //Serial.println("[LED ] OFF");
  digitalWrite(LED_BUILTIN, LOW);
}
