#include "LedControl.h"

bool LedControl::ledState = false;

void LedControl::testLED() {
  Serial.println("LED testi başlatılıyor...");
  int testPins[] = {2, 16};
  for (int pin : testPins) {
    Serial.println("Test ediliyor: GPIO" + String(pin));
    pinMode(pin, OUTPUT);
    for (int i = 0; i < 5; i++) {
      digitalWrite(pin, LOW);
      Serial.println("LED açık (GPIO" + String(pin) + ")");
      delay(500);
      digitalWrite(pin, HIGH);
      Serial.println("LED kapalı (GPIO" + String(pin) + ")");
      delay(500);
    }
  }
  pinMode(LED_WIFI, OUTPUT);
  digitalWrite(LED_WIFI, HIGH);
  Serial.println("LED testi tamamlandı!");
}

void LedControl::updateLEDs() {
  digitalWrite(LED_WIFI, ledState ? LOW : HIGH);
}

void LedControl::toggleLed() {
  ledState = !ledState;
}

bool LedControl::getLedState() {
  return ledState;
}