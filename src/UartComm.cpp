#include "UartComm.h"
#include "SystemUtils.h"
#include "SpiffsUtils.h"
#include <FS.h>

SoftwareSerial UartComm::tivaSerial(TIVA_RX_PIN, TIVA_TX_PIN);

const char* UartComm::updateFilePath = "/update.bin";

void UartComm::setup() {
  // UART pinlerini ayarla
  pinMode(TIVA_RX_PIN, INPUT);
  pinMode(TIVA_TX_PIN, OUTPUT);
  digitalWrite(TIVA_TX_PIN, HIGH);  // UART idle state
  
  // UART'ı başlat (SoftwareSerial üzerinden Tiva ile iletişim sağlanacak)
  tivaSerial.begin(TIVA_BAUD_RATE);
  delay(100);  // UART'ın stabil olması için bekle
  
  // Debug çıktısı USB seri portuna (Serial) gönderiliyor
  Serial.println("\n--- UART Başlatma ---");
  Serial.println("UART başlatıldı:");
  Serial.println("- Baud Rate: " + String(TIVA_BAUD_RATE));
  Serial.println("- RX Pin: " + String(TIVA_RX_PIN));
  Serial.println("- TX Pin: " + String(TIVA_TX_PIN));
  Serial.println("--- UART Hazır ---\n");
  SystemUtils::setStatusMessage("UART başlatıldı");
}

void UartComm::sendUartChar() {
  Serial.println("\n--- UART Başlatma İşlemi ---");
  tivaSerial.write(0xAA);
  Serial.println("TIVA'ya başlatma komutu (0xAA) gönderildi");
  SystemUtils::setStatusMessage("TIVA'ya başlatma komutu gönderildi");
  
  unsigned long startTime = millis();
  bool responseReceived = false;
  
  while (millis() - startTime < 2000 && !responseReceived) {
    if (tivaSerial.available()) {
      uint8_t response = tivaSerial.read();
      Serial.printf("TIVA'dan yanıt: 0x%02X\n", response);
      if (response == 0xAA) {
        Serial.println("TIVA bootloader moduna geçti (ACK alındı)");
        SystemUtils::setStatusMessage("TIVA bootloader moduna geçti");
        responseReceived = true;
      } else {
        Serial.printf("Hata: Beklenmeyen yanıt: 0x%02X\n", response);
        SystemUtils::setStatusMessage("Beklenmeyen yanıt alındı");
      }
    }
    yield();
  }
  
  if (!responseReceived) {
    Serial.println("Hata: TIVA'dan yanıt alınamadı (Timeout)");
    SystemUtils::setStatusMessage("TIVA yanıt vermedi");
  }
  
  Serial.println("--- UART Başlatma İşlemi Tamamlandı ---\n");
}

void UartComm::sendBytesViaUart() {
    File file = SPIFFS.open(updateFilePath, "r");
    if (!file) {
        Serial.println("Hata: Dosya açılamadı");
        return;
    }

    size_t fileSize = file.size();
    Serial.println("Dosya boyutu: " + String(fileSize) + " byte");

    tivaSerial.write(0xAA); // Başlangıç
    delay(100); // Tiva’nın hazırlanması için kısa bekleme

    // Dosya boyutunu gönder
    tivaSerial.write((uint8_t)(fileSize >> 24));
    tivaSerial.write((uint8_t)(fileSize >> 16));
    tivaSerial.write((uint8_t)(fileSize >> 8));
    tivaSerial.write((uint8_t)fileSize);
    delay(100);

    // Veriyi gönder
    uint8_t buffer[UART_BUFFER_SIZE];
    size_t bytesRead;
    while ((bytesRead = file.read(buffer, UART_BUFFER_SIZE)) > 0) {
        for (size_t i = 0; i < bytesRead; i++) {
            tivaSerial.write(buffer[i]);
            delay(1); // Veri akışını yavaşlatmak için
        }
    }
    tivaSerial.write(0xDD); // Bitiş
    //tivaSerial.write(DOUBLE_CHECK1);
    //tivaSerial.write(DOUBLE_CHECK2);

    file.close();
    Serial.println("Veri gönderimi tamamlandı!");
}

void UartComm::checkTivaResponse() {
  if (tivaSerial.available()) {
    Serial.println("\n--- TIVA'dan Yanıt Alındı ---");
    while (tivaSerial.available()) {
      uint8_t c = tivaSerial.read();
      Serial.printf("Alınan: 0x%02X\n", c);
      SystemUtils::setStatusMessage("TIVA'dan yanıt: 0x" + String(c, HEX));
    }
    Serial.println("--- TIVA Yanıt Sonu ---\n");
  }
}

uint8_t UartComm::calculateChecksum(uint8_t data) {
  // CRC-8 algoritması kullan
  uint8_t crc = 0;
  for (int i = 0; i < 8; i++) {
    if ((crc ^ data) & 0x01) {
      crc = (crc >> 1) ^ 0x8C;
    } else {
      crc >>= 1;
    }
    data >>= 1;
  }
  return crc;
}

uint8_t UartComm::calculateChecksum(uint8_t* data, uint8_t length) {
  uint8_t crc = 0;
  for (uint8_t i = 0; i < length; i++) {
    crc = calculateChecksum(data[i] ^ crc);
  }
  return crc;
}