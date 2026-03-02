#include "SystemUtils.h"

SystemState SystemUtils::currentState = SystemState::STATE_INIT;
String SystemUtils::errorMessage = "";
String SystemUtils::systemStatusMsg = "";
unsigned long SystemUtils::bootTime = 0;

void SystemUtils::setup() {
  bootTime = millis();
  Serial.begin(115200);
  Serial.println("NodeMCU Web Dosya Yükleyici başlatıldı");
  Serial.println("Firmware versiyonu: 1.0.0");
}

void SystemUtils::setState(SystemState state) {
  currentState = state;
}

SystemState SystemUtils::getState() {
  return currentState;
}

String SystemUtils::getStateText() {
  switch (currentState) {
    case SystemState::STATE_INIT: return "Başlatılıyor";
    case SystemState::STATE_WIFI_CONNECTING: return "WiFi Bağlanıyor...";
    case SystemState::STATE_IDLE: return "Hazır";
    case SystemState::STATE_UPLOADING: return "Dosya Yükleniyor...";
    case SystemState::STATE_ERROR: return "Hata: " + errorMessage;
    default: return "Bilinmeyen Durum";
  }
}

String SystemUtils::getSystemUptime() {
  unsigned long uptime = millis() - bootTime;
  unsigned long days = uptime / (24 * 60 * 60 * 1000);
  uptime %= (24 * 60 * 60 * 1000);
  unsigned long hours = uptime / (60 * 60 * 1000);
  uptime %= (60 * 60 * 1000);
  unsigned long minutes = uptime / (60 * 1000);
  uptime %= (60 * 1000);
  unsigned long seconds = uptime / 1000;
  char buffer[50];
  sprintf(buffer, "%lu gün %02lu:%02lu:%02lu", days, hours, minutes, seconds);
  return String(buffer);
}

String SystemUtils::getSystemStatus() {
  return systemStatusMsg;
}

void SystemUtils::setErrorMessage(const String& message) {
  errorMessage = message;
}

void SystemUtils::setStatusMessage(const String& message) {
  systemStatusMsg = message;
}

void SystemUtils::handleError() {
  static unsigned long errorStartTime = 0;
  if (errorStartTime == 0) {
    errorStartTime = millis();
    Serial.println("Hata durumu: " + errorMessage);
  }
  if (millis() - errorStartTime > 10000) {
    errorStartTime = 0;
    Serial.println("Yeniden başlatılıyor...");
    setState(SystemState::STATE_WIFI_CONNECTING);
  }
}