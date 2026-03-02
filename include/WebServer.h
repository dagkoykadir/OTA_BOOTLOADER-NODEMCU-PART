#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

class WebServer {
public:
  static void setupWiFi();
  static void setupWebServer();
  static void handleClient();

private:
  static ESP8266WebServer server;
  static const char* ssid;
  static const char* password;
  static const int WEB_SERVER_PORT = 80;
  static void handleRoot();
  static void handleUpload();
  static void handleStatus();
  static void handleRestart();
  static void handleLedToggle();
  static void handleClearSpiffs();
  static void handleSplitFile();
  static void handleSendBytes();
  static void handleSendUartChar();
  static void handleNotFound();
};

#endif