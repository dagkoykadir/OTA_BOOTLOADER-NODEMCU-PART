#include "SpiffsUtils.h"
#include "SystemUtils.h"

SpiffsInfo SpiffsUtils::spiffsInfo;
const char* SpiffsUtils::updateFilePath = "/update.bin";

size_t SpiffsUtils::getChunkCount(size_t* totalSize) {
  size_t count = 0;
  *totalSize = 0;
  
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    if (fileName.startsWith("/chunk_") && fileName.endsWith(".bin")) {
      count++;
      *totalSize += dir.fileSize();
    }
  }
  return count;
}

void SpiffsUtils::setup() {
  Serial.println("SPIFFS başlatılıyor...");
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS başlatılamadı, formatlanıyor...");
    if (SPIFFS.format()) {
      Serial.println("SPIFFS formatlandı, yeniden başlatılıyor...");
      if (!SPIFFS.begin()) {
        Serial.println("SPIFFS tekrar başlatılamadı!");
        SystemUtils::setErrorMessage("SPIFFS başlatma hatası: Flash bellek arızalı olabilir");
        SystemUtils::setState(SystemState::STATE_ERROR);
        return;
      }
    } else {
      Serial.println("SPIFFS formatlama başarısız!");
      SystemUtils::setErrorMessage("SPIFFS formatlama hatası: Flash bellek arızalı olabilir");
      SystemUtils::setState(SystemState::STATE_ERROR);
      return;
    }
  }
  clearSpiffs();
  checkSpiffsStatus();
  testSpiffs();
  Serial.printf("SPIFFS Toplam: %u bayt, Kullanılan: %u bayt, Boş: %u bayt\n",
                spiffsInfo.totalBytes, spiffsInfo.usedBytes, spiffsInfo.freeBytes);
}

void SpiffsUtils::checkSpiffsStatus() {
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  spiffsInfo.totalBytes = fs_info.totalBytes;
  spiffsInfo.usedBytes = fs_info.usedBytes;
  spiffsInfo.freeBytes = fs_info.totalBytes - fs_info.usedBytes;
}

void SpiffsUtils::clearSpiffs() {
  Serial.println("SPIFFS temizleniyor...");
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    Serial.println("Siliniyor: " + fileName);
    SPIFFS.remove(fileName);
  }
  Serial.println("SPIFFS temizlendi!");
  checkSpiffsStatus();
}

void SpiffsUtils::testSpiffs() {
  Serial.println("SPIFFS testi başlatılıyor...");
  for (int i = 0; i < 3; i++) {
    String testPath = "/test" + String(i) + ".txt";
    File testFile = SPIFFS.open(testPath, "w");
    if (!testFile) {
      Serial.println("SPIFFS test dosyası açılamadı: " + testPath);
      SystemUtils::setErrorMessage("SPIFFS test hatası: Dosya oluşturulamadı");
      SystemUtils::setState(SystemState::STATE_ERROR);
      return;
    }
    if (testFile.print("Test" + String(i))) {
      Serial.println("SPIFFS test yazma başarılı: " + testPath);
    } else {
      Serial.println("SPIFFS test yazma başarısız: " + testPath);
      SystemUtils::setErrorMessage("SPIFFS test hatası: Yazma başarısız");
      SystemUtils::setState(SystemState::STATE_ERROR);
      testFile.close();
      return;
    }
    testFile.close();
    if (!SPIFFS.remove(testPath)) {
      Serial.println("SPIFFS test dosyası silinemedi: " + testPath);
      SystemUtils::setErrorMessage("SPIFFS test hatası: Dosya silinemedi");
      SystemUtils::setState(SystemState::STATE_ERROR);
      return;
    } else {
      Serial.println("SPIFFS test dosyası silindi: " + testPath);
    }
  }
  Serial.println("SPIFFS testi tamamlandı!");
}

String SpiffsUtils::listSpiffsFiles() {
  String fileList = "[";
  Dir dir = SPIFFS.openDir("/");
  bool first = true;
  while (dir.next()) {
    if (!first) fileList += ",";
    fileList += "{\"name\":\"" + String(dir.fileName()) + "\",\"size\":" + String(dir.fileSize()) + "}";
    first = false;
  }
  fileList += "]";
  return fileList;
}

SpiffsInfo SpiffsUtils::getSpiffsInfo() {
  return spiffsInfo;
}

bool SpiffsUtils::splitFileIntoChunks(const char* sourceFile, size_t chunkSize) {
  // Kaynak dosyayı kontrol et
  if (!SPIFFS.exists(sourceFile)) {
    Serial.println("Hata: Bölünecek dosya bulunamadı: " + String(sourceFile));
    SystemUtils::setStatusMessage("Bölünecek dosya bulunamadı");
    return false;
  }
  
  // Kaynak dosyayı aç
  File file = SPIFFS.open(sourceFile, "r");
  if (!file) {
    Serial.println("Hata: Dosya açılamadı: " + String(sourceFile));
    SystemUtils::setStatusMessage("Dosya açılamadı");
    return false;
  }
  
  // Dosya boyutunu al
  size_t fileSize = file.size();
  Serial.println("Dosya boyutu: " + String(fileSize) + " bayt");
  
  // Önceki chunk dosyalarını temizle
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    if (fileName.startsWith("/chunk_") && fileName.endsWith(".bin")) {
      SPIFFS.remove(fileName);
      Serial.println("Eski chunk dosyası silindi: " + fileName);
    }
  }
  
  // Dosyayı parçalara böl
  size_t chunkCount = (fileSize + chunkSize - 1) / chunkSize; // Yukarı yuvarlama
  Serial.println("Dosya " + String(chunkCount) + " parçaya bölünecek (her biri max " + String(chunkSize) + " bayt)");
  
  // Yeterli alan kontrolü yap
  checkSpiffsStatus();
  if (spiffsInfo.freeBytes < fileSize) {
    Serial.println("Hata: Parçaları kaydetmek için yeterli alan yok!");
    SystemUtils::setStatusMessage("Yeterli depolama alanı yok");
    file.close();
    return false;
  }
  
  // Parçaları oluştur
  bool success = true;
  uint8_t buffer[chunkSize];
  size_t bytesRead = 0;
  size_t totalBytesRead = 0;
  
  for (size_t i = 0; i < chunkCount && success; i++) {
    String chunkFileName = "/chunk_" + String(i) + ".bin";
    File chunkFile = SPIFFS.open(chunkFileName, "w");
    if (!chunkFile) {
      Serial.println("Hata: Chunk dosyası oluşturulamadı: " + chunkFileName);
      success = false;
      break;
    }
    
    // Bu parça için okunacak boyutu belirle
    size_t bytesToRead = min(chunkSize, fileSize - totalBytesRead);
    bytesRead = file.read(buffer, bytesToRead);
    
    if (bytesRead != bytesToRead) {
      Serial.println("Hata: Dosya okuma hatası!");
      success = false;
      chunkFile.close();
      break;
    }
    
    // Chunk dosyasına yaz
    if (chunkFile.write(buffer, bytesRead) != bytesRead) {
      Serial.println("Hata: Chunk dosyasına yazma hatası!");
      success = false;
      chunkFile.close();
      break;
    }
    
    chunkFile.close();
    totalBytesRead += bytesRead;
    Serial.println("Chunk " + String(i) + " oluşturuldu: " + String(bytesRead) + " bayt");
    yield(); // ESP8266 watchdog için
  }
  
  file.close();
  
  if (success) {
    Serial.println("Dosya başarıyla " + String(chunkCount) + " parçaya bölündü");
    SystemUtils::setStatusMessage("Dosya " + String(chunkCount) + " parçaya bölündü");
  } else {
    Serial.println("Dosya bölme işlemi başarısız!");
    SystemUtils::setStatusMessage("Dosya bölme hatası");
    
    // Hata durumunda oluşturulan tüm chunk dosyalarını sil
    dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      if (fileName.startsWith("/chunk_") && fileName.endsWith(".bin")) {
        SPIFFS.remove(fileName);
        Serial.println("Hatalı chunk dosyası silindi: " + fileName);
      }
    }
  }
  
  return success;
}