#ifndef UART_COMM_H
#define UART_COMM_H

#include <Arduino.h>
#include <SoftwareSerial.h>


#define UART_BUFFER_SIZE 128
#define START_MARKER     0XAA
#define END_MARKER       0XDD
#define ACK              0X06
#define NACK             0x07
#define DOUBLE_CHECK1    'f'
#define DOUBLE_CHECK2    'k'


typedef struct {
    uint8_t blockNum;
    uint8_t blockSize;
    uint8_t data[UART_BUFFER_SIZE];
    uint8_t checksum;
} UartPacket;



class UartComm {
public:
  static void setup();
  static void sendUartChar();
  static void sendBytesViaUart();
  static void checkTivaResponse();
private:
  static SoftwareSerial tivaSerial;
  static const int TIVA_RX_PIN = 13;
  static const int TIVA_TX_PIN = 15;
  static const int TIVA_BAUD_RATE = 115200;
  static const int UART_DELAY_MS = 10;
  static const char* updateFilePath;
  static uint8_t calculateChecksum(uint8_t data);
  static uint8_t calculateChecksum(uint8_t* data, uint8_t length);
};

#endif