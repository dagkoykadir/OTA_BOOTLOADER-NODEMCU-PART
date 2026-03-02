#include <Arduino.h>
#include "WebServer.h"
#include "SpiffsUtils.h"
#include "UartComm.h"
#include "SystemUtils.h"
#include "LedControl.h"

void setup() {
  SystemUtils::setup();
  LedControl::testLED();
  SpiffsUtils::setup();
  UartComm::setup();
  SystemUtils::setState(SystemState::STATE_WIFI_CONNECTING);
}

void loop() {
  switch (SystemUtils::getState()) {
    case SystemState::STATE_INIT:
      break;
    case SystemState::STATE_WIFI_CONNECTING:
      WebServer::setupWiFi();
      break;
    case SystemState::STATE_IDLE:
      break;
    case SystemState::STATE_UPLOADING:
      break;
    case SystemState::STATE_ERROR:
      SystemUtils::handleError();
      break;
  }
  LedControl::updateLEDs();
  WebServer::handleClient();
  UartComm::checkTivaResponse();
  yield();
}