#ifndef SYSTEM_UTILS_H
#define SYSTEM_UTILS_H

#include <Arduino.h>

enum class SystemState {
  STATE_INIT,
  STATE_WIFI_CONNECTING,
  STATE_IDLE,
  STATE_UPLOADING,
  STATE_ERROR
};

class SystemUtils {
public:
  static void setup();
  static void setState(SystemState state);
  static SystemState getState();
  static String getStateText();
  static String getSystemUptime();
  static String getSystemStatus();
  static void setErrorMessage(const String& message);
  static void setStatusMessage(const String& message);
  static void handleError();

private:
  static SystemState currentState;
  static String errorMessage;
  static String systemStatusMsg;
  static unsigned long bootTime;
};

#endif