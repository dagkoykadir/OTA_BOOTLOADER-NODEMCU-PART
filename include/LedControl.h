#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Arduino.h>

class LedControl {
public:
  static void testLED();
  static void updateLEDs();
  static void toggleLed();
  static bool getLedState();

private:
  static const int LED_WIFI = 2;
  static bool ledState;
};

#endif