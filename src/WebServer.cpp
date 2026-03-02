#include "WebServer.h"
#include "SystemUtils.h"
#include "SpiffsUtils.h"
#include "UartComm.h"
#include "LedControl.h"

ESP8266WebServer WebServer::server(WEB_SERVER_PORT);
const char* WebServer::ssid = "TurkTelekom_TPEC60";
const char* WebServer::password = "fkdrmyKFHUE3";

void WebServer::setupWiFi() {
  static unsigned long wifiStartTime = 0;
  static bool wifiBegun = false;

  if (!wifiBegun) {
    Serial.println("WiFi bağlantısı başlatılıyor...");
    WiFi.begin(ssid, password);
    wifiStartTime = millis();
    wifiBegun = true;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi bağlantısı başarılı");
    Serial.print("IP adresi: ");
    Serial.println(WiFi.localIP());
    SystemUtils::setState(SystemState::STATE_IDLE);
    wifiBegun = false;
    setupWebServer();
  } else if (millis() - wifiStartTime > 20000) {
    Serial.println("WiFi bağlantısı başarısız, yeniden deneniyor...");
    WiFi.disconnect();
    wifiBegun = false;
    wifiStartTime = millis();
  }
}

void WebServer::setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/upload", HTTP_POST, []() { server.send(200); }, handleUpload);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/restart", HTTP_GET, handleRestart);
  server.on("/led-toggle", HTTP_GET, handleLedToggle);
  server.on("/clear-spiffs", HTTP_GET, handleClearSpiffs);
  server.on("/send-bytes", HTTP_GET, handleSendBytes);
  server.on("/send-uart-char", HTTP_GET, handleSendUartChar);
  server.on("/chunk-info", HTTP_GET, []() {
    size_t totalSize = 0;
    size_t chunkCount = SpiffsUtils::getChunkCount(&totalSize);
    String jsonResponse = "{";
    jsonResponse += "\"chunks\":" + String(chunkCount) + ",";
    jsonResponse += "\"totalSize\":" + String(totalSize) + ",";
    jsonResponse += "\"chunkSize\":" + String(SpiffsUtils::DEFAULT_CHUNK_SIZE);
    jsonResponse += "}";
    server.send(200, "application/json", jsonResponse);
  });
  server.on("/files", HTTP_GET, []() {
    server.send(200, "application/json", SpiffsUtils::listSpiffsFiles());
  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Web sunucusu başlatıldı");
}

void WebServer::handleClient() {
  server.handleClient();
}

void WebServer::handleRoot() {
  SpiffsUtils::checkSpiffsStatus();
  
  String html = "<!DOCTYPE html>";
  html += "<html lang='tr'>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>NodeMCU Kontrol Paneli</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 0; padding: 10px; color: #e0e0e0; background-color: #1a1a1a; height: 100vh; }";
  html += ".container { max-width: 1200px; margin: 0 auto; background-color: #2d2d2d; padding: 15px; border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.3); height: calc(100vh - 20px); }";
  html += "h1 { color: #4CAF50; text-align: center; margin: 0 0 15px 0; font-size: 24px; }";
  html += "h2 { color: #4CAF50; border-bottom: 2px solid #404040; padding-bottom: 5px; margin: 10px 0; font-size: 18px; }";
  html += ".main-content { display: flex; gap: 15px; height: calc(100% - 40px); }";
  html += ".left-panel { flex: 1; overflow-y: auto; }";
  html += ".right-panel { flex: 2; overflow-y: auto; }";
  html += ".status-box { background-color: #363636; border: 1px solid #404040; padding: 10px; border-radius: 8px; margin-bottom: 10px; }";
  html += ".status-item { margin-bottom: 8px; padding: 5px; background-color: #2d2d2d; border-radius: 4px; font-size: 14px; }";
  html += ".status-label { font-weight: bold; display: inline-block; width: 150px; color: #4CAF50; }";
  html += ".button { background-color: #4CAF50; color: white; border: none; padding: 8px 15px; text-align: center; text-decoration: none; display: inline-block; font-size: 14px; margin: 4px 2px; cursor: pointer; border-radius: 6px; transition: all 0.3s; }";
  html += ".button:hover { background-color: #45a049; transform: translateY(-2px); }";
  html += ".button-green { background-color: #4CAF50; }";
  html += ".button-red { background-color: #f44336; }";
  html += ".button-orange { background-color: #ff9800; }";
  html += ".progress { margin-top: 10px; width: 100%; background-color: #404040; border-radius: 8px; overflow: hidden; }";
  html += ".progress-bar { height: 20px; background-color: #4CAF50; border-radius: 8px; text-align: center; line-height: 20px; color: white; transition: width 0.3s; font-size: 12px; }";
  html += ".file-list { margin-top: 10px; background-color: #363636; border-radius: 8px; padding: 10px; max-height: 150px; overflow-y: auto; }";
  html += ".file-item { padding: 8px; border-bottom: 1px solid #404040; background-color: #2d2d2d; margin-bottom: 4px; border-radius: 4px; font-size: 14px; }";
  html += ".file-item:last-child { border-bottom: none; margin-bottom: 0; }";
  html += ".section { background-color: #363636; padding: 10px; border-radius: 8px; margin-bottom: 10px; }";
  html += ".footer { text-align: center; margin-top: 10px; padding-top: 10px; border-top: 1px solid #404040; color: #888; font-size: 12px; }";
  html += "input[type='file'] { margin: 5px 0; color: #e0e0e0; font-size: 14px; }";
  html += "input[type='submit'] { margin-top: 5px; }";
  html += ".button-group { display: flex; flex-wrap: wrap; gap: 5px; }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>NodeMCU Kontrol Paneli</h1>";
  
  html += "<div class='main-content'>";
  html += "<div class='left-panel'>";
  html += "<div class='section'>";
  html += "<div class='status-box'>";
  html += "<h2>Sistem Durumu</h2>";
  html += "<div class='status-item'><span class='status-label'>Durum:</span> <span id='currentState'>" + SystemUtils::getStateText() + "</span></div>";
  html += "<div class='status-item'><span class='status-label'>Çalışma Süresi:</span> <span id='uptime'>" + SystemUtils::getSystemUptime() + "</span></div>";
  html += "<div class='status-item'><span class='status-label'>IP Adresi:</span> " + WiFi.localIP().toString() + "</div>";
  html += "<div class='status-item'><span class='status-label'>WiFi SSID:</span> " + String(ssid) + "</div>";
  html += "<div class='status-item'><span class='status-label'>WiFi Sinyal:</span> " + String(WiFi.RSSI()) + " dBm</div>";
  html += "<div class='status-item'><span class='status-label'>SPIFFS Toplam:</span> <span id='spiffsTotal'>" + String(SpiffsUtils::getSpiffsInfo().totalBytes / 1024) + " KB</span></div>";
  html += "<div class='status-item'><span class='status-label'>SPIFFS Boş:</span> <span id='spiffsFree'>" + String(SpiffsUtils::getSpiffsInfo().freeBytes / 1024) + " KB</span></div>";
  html += "<div class='status-item'><span class='status-label'>Sistem Mesajı:</span> <span id='statusMsg'>" + SystemUtils::getSystemStatus() + "</span></div>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  
  html += "<div class='right-panel'>";
  html += "<div class='section'>";
  html += "<h2>LED Kontrolü</h2>";
  html += "<div class='button-group'>";
  html += "<button id='ledButton' class='button' onclick='toggleLed()'>";
  html += LedControl::getLedState() ? "LED Kapat" : "LED Aç";
  html += "</button>";
  html += "</div>";
  html += "</div>";
  
  html += "<div class='section'>";
  html += "<h2>Dosya Yükleme</h2>";
  html += "<form method='POST' action='/upload' enctype='multipart/form-data' id='upload_form'>";
  html += "<input type='file' name='update' accept='.bin'>";
  html += "<input type='submit' value='Dosya Yükle' class='button button-green'>";
  html += "</form>";
  html += "<div class='progress'>";
  html += "<div class='progress-bar' id='progress-bar' style='width: 0%'>0%</div>";
  html += "</div>";
  html += "</div>";
  
  html += "<div class='file-list' id='fileList'></div>";
  
  html += "<div class='section'>";
  html += "<h2>UART İşlemleri</h2>";
  html += "<div class='button-group'>";
  html += "<button onclick='sendBytes()' class='button button-orange'>Baytları Gönder</button>";
  html += "<button onclick='sendUartChar()' class='button button-orange'>UART Başlat</button>";
  html += "</div>";
  html += "</div>";
  
  html += "<div class='section'>";
  html += "<div class='button-group'>";
  html += "<button onclick='clearSpiffs()' class='button button-red'>SPIFFS'i Boşalt</button>";
  html += "<button onclick='restartEsp()' class='button button-red'>Yeniden Başlat</button>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  
  html += "<div class='footer'>NodeMCU Kontrol Paneli - v1.0.0</div>";
  
  html += "<script>";
  html += "var ledState = " + String(LedControl::getLedState()) + ";";
  html += "var form = document.getElementById('upload_form');";
  html += "var bar = document.getElementById('progress-bar');";
  html += "var ledButton = document.getElementById('ledButton');";
  html += "var fileList = document.getElementById('fileList');";
  html += "var spiffsTotal = document.getElementById('spiffsTotal');";
  html += "var spiffsFree = document.getElementById('spiffsFree');";
  html += "var isFileUploaded = false;";
  html += "var originalFileName = '';";
  html += "var sendBytesButton = document.querySelector('button[onclick=\"sendBytes()\"]');";
  html += "sendBytesButton.disabled = true;";
  html += "sendBytesButton.style.opacity = '0.5';";
  html += "sendBytesButton.style.cursor = 'not-allowed';";
  
  html += "function toggleLed() {";
  html += "  fetch('/led-toggle').then(() => {";
  html += "    ledState = !ledState;";
  html += "    ledButton.innerText = ledState ? 'LED Kapat' : 'LED Aç';";
  html += "    updateStatus();";
  html += "  });";
  html += "}";
  
  html += "form.addEventListener('submit', function(e) {";
  html += "  e.preventDefault();";
  html += "  var file = this.elements.update.files[0];";
  html += "  if (!file) {";
  html += "    alert('Lütfen bir .bin dosyası seçin!');";
  html += "    return;";
  html += "  }";
  html += "  originalFileName = file.name;";
  html += "  localStorage.setItem('uploadedFileName', file.name);";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.upload.addEventListener('progress', function(e) {";
  html += "    if (e.lengthComputable) {";
  html += "      var percent = Math.round((e.loaded / e.total) * 100);";
  html += "      bar.style.width = percent + '%';";
  html += "      bar.innerText = percent + '%';";
  html += "    }";
  html += "  });";
  html += "  xhr.addEventListener('load', function() {";
  html += "    if (xhr.status === 200) {";
  html += "      alert('Dosya başarıyla yüklendi! Depolama alanı kontrol ediliyor...');";
  html += "      bar.style.width = '0%';";
  html += "      bar.innerText = '0%';";
  html += "      isFileUploaded = true;";
  html += "      sendBytesButton.disabled = false;";
  html += "      sendBytesButton.style.opacity = '1';";
  html += "      sendBytesButton.style.cursor = 'pointer';";
  html += "      updateStatus();";
  html += "      updateFiles();";
  html += "    } else {";
  html += "      alert('Dosya yükleme hatası: ' + xhr.responseText);";
  html += "      isFileUploaded = false;";
  html += "      sendBytesButton.disabled = true;";
  html += "      sendBytesButton.style.opacity = '0.5';";
  html += "      sendBytesButton.style.cursor = 'not-allowed';";
  html += "    }";
  html += "  });";
  html += "  xhr.addEventListener('error', function() {";
  html += "    alert('Ağ hatası: Sunucuya bağlanılamadı');";
  html += "  });";
  html += "  xhr.addEventListener('timeout', function() {";
  html += "    alert('Zaman aşımı: Sunucuya ulaşılamadı');";
  html += "  });";
  html += "  xhr.timeout = 30000;";
  html += "  xhr.open('POST', '/upload');";
  html += "  var formData = new FormData();";
  html += "  formData.append('update', file);";
  html += "  xhr.send(formData);";
  html += "});";
  
  html += "function restartEsp() {";
  html += "  fetch('/restart').then(() => {";
  html += "    setTimeout(() => { window.location.reload(); }, 5000);";
  html += "  }).catch(error => {";
  html += "    alert('Yeniden başlatma hatası: ' + error);";
  html += "  });";
  html += "}";
  
  html += "function clearSpiffs() {";
  html += "  if (confirm('Tüm SPIFFS dosyalarını silmek istediğinizden emin misiniz?')) {";
  html += "    fetch('/clear-spiffs').then(response => {";
  html += "      if (response.ok) {";
  html += "        alert('SPIFFS başarıyla boşaltıldı!');";
  html += "        updateStatus();";
  html += "        updateFiles();";
  html += "      } else {";
  html += "        alert('SPIFFS boşaltma hatası!');";
  html += "      }";
  html += "    }).catch(error => {";
  html += "      alert('Ağ hatası: ' + error);";
  html += "    });";
  html += "  }";
  html += "}";
  
  html += "function sendBytes() {";
  html += "  if (confirm('Baytlar UART üzerinden gönderilecek. TIVA bootloader modunda hazır mı?')) {";
  html += "    fetch('/send-bytes').then(response => {";
  html += "      if (response.ok) {";
  html += "        alert('Baytlar başarıyla gönderildi!');";
  html += "        updateStatus();";
  html += "      } else {";
  html += "        alert('Bayt gönderme hatası!');";
  html += "      }";
  html += "    }).catch(error => {";
  html += "      alert('Ağ hatası: ' + error);";
  html += "    });";
  html += "  }";
  html += "}";
  
  html += "function sendUartChar() {";
  html += "  fetch('/send-uart-char').then(response => {";
  html += "    if (response.ok) {";
  html += "      alert('Başlatma komutu gönderildi');";
  html += "      updateStatus();";
  html += "    } else {";
  html += "      alert('Karakter gönderme hatası!');";
  html += "    }";
  html += "  }).catch(error => {";
  html += "    alert('Ağ hatası: ' + error);";
  html += "  });";
  html += "}";
  
  html += "function updateStatus() {";
  html += "  fetch('/status')";
  html += "    .then(response => response.json())";
  html += "    .then(data => {";
  html += "      document.getElementById('currentState').innerText = data.state;";
  html += "      document.getElementById('uptime').innerText = data.uptime;";
  html += "      document.getElementById('statusMsg').innerText = data.statusMsg;";
  html += "      spiffsTotal.innerText = data.spiffsTotal;";
  html += "      spiffsFree.innerText = data.spiffsFree;";
  html += "    })";
  html += "    .catch(error => console.error('Status güncelleme hatası:', error));";
  html += "}";
  
  html += "function updateFiles() {";
  html += "  fetch('/files')";
  html += "    .then(response => response.json())";
  html += "    .then(data => {";
  html += "      fileList.innerHTML = '<h2>Dosya Listesi</h2><div>';";
  html += "      data.forEach(file => {";
  html += "        let fileName = file.name;";
  html += "        if (fileName === '/update.bin') {";
  html += "          fileName = localStorage.getItem('uploadedFileName') || 'Yüklenen Dosya';";
  html += "        }";
  html += "        fileList.innerHTML += '<div class=\"file-item\">' + fileName + ' (' + file.size + ' bayt)</div>';";
  html += "      });";
  html += "      fileList.innerHTML += '</div>';";
  html += "    })";
  html += "    .catch(error => console.error('Dosya listesi güncelleme hatası:', error));";
  html += "}";
  
  html += "setInterval(updateStatus, 5000);";
  html += "setInterval(updateFiles, 5000);";
  html += "updateStatus();";
  html += "updateFiles();";
  
  html += "</script>";
  
  html += "</body>";
  html += "</html>";
  
  server.send(200, "text/html", html);
}

void WebServer::handleUpload() {
  HTTPUpload& upload = server.upload();
  static File file;
  static size_t initialFreeBytes = 0;

  if (upload.status == UPLOAD_FILE_START) {
    SystemUtils::setState(SystemState::STATE_UPLOADING);
    SystemUtils::setStatusMessage("Dosya yükleniyor...");
    Serial.println("Dosya yükleme başlatılıyor: " + String(upload.filename));

    // Clear any existing chunk files
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      if (fileName.startsWith("/chunk_") && fileName.endsWith(".bin")) {
        SPIFFS.remove(fileName);
        Serial.println("Eski chunk dosyası silindi: " + fileName);
      }
    }

    SpiffsUtils::checkSpiffsStatus();
    initialFreeBytes = SpiffsUtils::getSpiffsInfo().freeBytes;
    if (upload.totalSize > initialFreeBytes) {
      Serial.println("Hata: Yetersiz SPIFFS alanı. Gerekli: " + String(upload.totalSize) + " bayt, Boş: " + String(initialFreeBytes) + " bayt");
      server.send(500, "application/json", "{\"message\": \"Yetersiz SPIFFS alanı! Gerekli: " + String(upload.totalSize) + " bayt, Boş: " + String(initialFreeBytes) + " bayt\"}");
      SystemUtils::setState(SystemState::STATE_ERROR);
      SystemUtils::setErrorMessage("Yetersiz SPIFFS alanı");
      return;
    }

    if (SPIFFS.exists("/update.bin")) {
      if (!SPIFFS.remove("/update.bin")) {
        Serial.println("Hata: Eski dosya silinemedi: /update.bin");
        server.send(500, "application/json", "{\"message\": \"Eski dosya silinemedi!\"}");
        SystemUtils::setState(SystemState::STATE_ERROR);
        SystemUtils::setErrorMessage("Eski dosya silme hatası");
        return;
      }
      Serial.println("Eski dosya silindi: /update.bin");
    }

    file = SPIFFS.open("/update.bin", "w");
    if (!file) {
      Serial.println("Hata: Dosya açılamadı: /update.bin");
      server.send(500, "application/json", "{\"message\": \"Dosya açılamadı! SPIFFS dosya sistemi veya flash belleği kontrol edin.\"}");
      SystemUtils::setState(SystemState::STATE_ERROR);
      SystemUtils::setErrorMessage("SPIFFS dosya açma hatası: Flash bellek arızalı olabilir");
      return;
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (file) {
      size_t bytesWritten = file.write(upload.buf, upload.currentSize);
      if (bytesWritten != upload.currentSize) {
        Serial.println("Hata: Dosyaya yazma başarısız, yazılan: " + String(bytesWritten) + ", beklenen: " + String(upload.currentSize));
        file.close();
        server.send(500, "application/json", "{\"message\": \"Dosyaya yazma başarısız!\"}");
        SystemUtils::setState(SystemState::STATE_ERROR);
        SystemUtils::setErrorMessage("Dosyaya yazma hatası: Flash bellek arızalı olabilir");
        return;
      }
      file.flush();
      yield();
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (file) {
      file.close();
      Serial.println("Dosya yükleme tamamlandı, boyut: " + String(upload.totalSize) + " bayt");

      SpiffsUtils::checkSpiffsStatus();
      size_t expectedFreeBytes = initialFreeBytes - upload.totalSize;
      Serial.printf("Yükleme sonrası SPIFFS durumu - Toplam: %u bayt, Kullanılan: %u bayt, Boş: %u bayt\n",
                    SpiffsUtils::getSpiffsInfo().totalBytes, SpiffsUtils::getSpiffsInfo().usedBytes, SpiffsUtils::getSpiffsInfo().freeBytes);
      if (abs((long)SpiffsUtils::getSpiffsInfo().freeBytes - (long)expectedFreeBytes) <= 1024) {
        Serial.println("Doğrulama başarılı: Dosya SPIFFS'e doğru şekilde yazıldı");
        server.send(200, "application/json", "{\"message\": \"Dosya başarıyla yüklendi ve doğrulandı! Boyut: " + String(upload.totalSize) + " bayt\"}");
      } else {
        Serial.println("Doğrulama başarısız: SPIFFS alanı beklenen şekilde değişmedi. Beklenen boş alan: " + String(expectedFreeBytes) + " bayt, Gerçek: " + String(SpiffsUtils::getSpiffsInfo().freeBytes) + " bayt");
        server.send(500, "application/json", "{\"message\": \"Dosya yüklendi ancak doğrulama başarısız! SPIFFS alanı beklenen şekilde değişmedi.\"}");
        SystemUtils::setState(SystemState::STATE_ERROR);
        SystemUtils::setErrorMessage("Dosya doğrulama hatası: SPIFFS alanı beklenenden farklı");
        return;
      }

      SystemUtils::setState(SystemState::STATE_IDLE);
      SystemUtils::setStatusMessage("Dosya yüklendi ve doğrulandı");
    } else {
      Serial.println("Hata: Dosya kaydedilemedi!");
      server.send(500, "application/json", "{\"message\": \"Dosya kaydedilemedi! SPIFFS dosya sistemi veya flash belleği kontrol edin.\"}");
      SystemUtils::setState(SystemState::STATE_ERROR);
      SystemUtils::setErrorMessage("Dosya kaydetme hatası: Flash bellek arızalı olabilir");
    }
  }
  yield();
}

void WebServer::handleStatus() {
  SpiffsUtils::checkSpiffsStatus();
  String jsonStatus = "{";
  jsonStatus += "\"state\":\"" + SystemUtils::getStateText() + "\",";
  jsonStatus += "\"uptime\":\"" + SystemUtils::getSystemUptime() + "\",";
  jsonStatus += "\"statusMsg\":\"" + SystemUtils::getSystemStatus() + "\",";
  jsonStatus += "\"spiffsTotal\":\"" + String(SpiffsUtils::getSpiffsInfo().totalBytes / 1024) + " KB\",";
  jsonStatus += "\"spiffsFree\":\"" + String(SpiffsUtils::getSpiffsInfo().freeBytes / 1024) + " KB\"";
  jsonStatus += "}";
  server.send(200, "application/json", jsonStatus);
}

void WebServer::handleRestart() {
  SystemUtils::setStatusMessage("Yeniden başlatılıyor...");
  server.send(200, "text/plain", "Yeniden başlatılıyor...");
  delay(1000);
  ESP.restart();
}

void WebServer::handleLedToggle() {
  LedControl::toggleLed();
  server.send(200, "text/plain", LedControl::getLedState() ? "LED Açık" : "LED Kapalı");
}

void WebServer::handleClearSpiffs() {
  SpiffsUtils::clearSpiffs();
  SpiffsUtils::checkSpiffsStatus();
  SystemUtils::setStatusMessage("SPIFFS başarıyla boşaltıldı");
  server.send(200, "text/plain", "SPIFFS boşaltıldı, boş alan: " + String(SpiffsUtils::getSpiffsInfo().freeBytes / 1024) + " KB");
}

void WebServer::handleSplitFile() {
  Serial.println("\n--- Dosya Bölme İşlemi Başlatılıyor ---");
  if (!SPIFFS.exists("/update.bin")) {
    Serial.println("Hata: Bölünecek dosya bulunamadı: /update.bin");
    SystemUtils::setStatusMessage("Dosya bulunamadı: /update.bin");
    server.send(400, "application/json", "{\"message\": \"Hata: Bölünecek dosya bulunamadı. Lütfen önce bir .bin dosyası yükleyin.\"}");
    return;
  }

  // Önce mevcut chunk dosyalarını temizle
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    if (fileName.startsWith("/chunk_") && fileName.endsWith(".bin")) {
      SPIFFS.remove(fileName);
      Serial.println("Eski chunk dosyası silindi: " + fileName);
    }
  }

  Serial.println("Dosya " + String(SpiffsUtils::DEFAULT_CHUNK_SIZE) + " byte'lık parçalara bölünecek");
  
  // Dosyayı parçalara böl
  bool success = SpiffsUtils::splitFileIntoChunks("/update.bin");
  
  if (success) {
    // Chunk dosyalarını say ve toplam boyutu hesapla
    size_t totalChunks = 0;
    size_t totalSize = 0;
    dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      if (fileName.startsWith("/chunk_") && fileName.endsWith(".bin")) {
        totalChunks++;
        totalSize += dir.fileSize();
        Serial.println(fileName + " -> " + String(dir.fileSize()) + " byte");
      }
    }
    
    String message = "Dosya " + String(totalChunks) + " parçaya bölündü (Toplam: " + String(totalSize) + " byte)";
    Serial.println("\n" + message);
    Serial.println("Her parça " + String(SpiffsUtils::DEFAULT_CHUNK_SIZE) + " byte");
    Serial.println("Toplam " + String(totalSize) + " byte");
    Serial.println("Bölme işlemi başarılı!");
    
    SystemUtils::setStatusMessage(message);
    server.send(200, "application/json", "{\"message\": \"" + message + "\", \"chunks\": " + String(totalChunks) + ", \"size\": " + String(totalSize) + "}");
  } else {
    String errorMsg = "Dosya bölme işlemi başarısız oldu";
    Serial.println("\n" + errorMsg);
    SystemUtils::setStatusMessage(errorMsg);
    server.send(500, "application/json", "{\"message\": \"" + errorMsg + "\"}");
  }
  Serial.println("--- Dosya Bölme İşlemi Tamamlandı ---\n");
}

// void WebServer::handleSendBytes() {
//   Serial.println("\n--- UART Üzerinden Bayt Gönderme İşlemi Başlatılıyor ---");
//   UartComm::sendBytesViaUart();
//   String status = SystemUtils::getSystemStatus();
//   Serial.println("Durum: " + status);
//   server.send(200, "text/plain", "Baytlar gönderildi: " + status);
//   Serial.println("--- UART Gönderme İşlemi Tamamlandı ---\n");
// }

void WebServer::handleSendBytes() {
    Serial.println("\n--- UART Üzerinden Bayt Gönderme İşlemi Başlatılıyor ---");
    UartComm::sendBytesViaUart();
    String status = SystemUtils::getSystemStatus();
    Serial.println("Durum: " + status);
    server.send(200, "text/plain", "Baytlar gönderildi: " + status);
    Serial.println("--- UART Gönderme İşlemi Tamamlandı ---\n");
}

void WebServer::handleSendUartChar() {
  Serial.println("\n--- UART Üzerinden Karakter Gönderiliyor ---");
  UartComm::sendUartChar();
  Serial.println("Baslatma komutu gönderildi");
  server.send(200, "text/plain", "Karakter gönderildi");
  Serial.println("--- UART Karakter Gönderme İşlemi Tamamlandı ---\n");
}

void WebServer::handleNotFound() {
  server.send(404, "text/plain", "Sayfa bulunamadı!");
}