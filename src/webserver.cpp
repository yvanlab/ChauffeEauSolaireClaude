#include "webserver.h"
#include "config.h"
#include "logger.h"
#include "time_utils.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <Update.h>
#include <ESPmDNS.h>
#include <esp_task_wdt.h>
#include <vector> // Required for std::vector
#include <algorithm> // Required for std::sort

// Forward declarations
bool parseAddress(const String& addrStr, uint8_t* addr);

// External sensor variables (defined in main.cpp)
extern DallasTemperature sensors;
extern int sensorCount;
extern float dayPumpHours;
extern uint8_t airSensor[8], spaSensor[8], panelSensor[8];

// External configuration manager (defined in main.cpp)
extern const char* getResetReason();
extern ConfigManager configManager;
extern void setRelay(bool on);

// External version info (defined in main.cpp)
extern const char* FIRMWARE_VERSION;
extern const char* BUILD_DATE;
extern const char* BUILD_TIME;

// WiFi event handler for connection monitoring
static void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch(event) {
    case ARDUINO_EVENT_WIFI_STA_START:
      logger.info("WiFi station started");
      break;

    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      logger.success("WiFi connected to AP");
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      logger.successf("WiFi got IP: %s", WiFi.localIP().toString().c_str());
      logger.infof("Signal strength: %d dBm", WiFi.RSSI());
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      logger.warning("WiFi disconnected - auto-reconnect will attempt to restore connection");
      break;

    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      logger.error("WiFi lost IP address");
      break;

    default:
      break;
  }
}

WebServerManager::WebServerManager(SpaConfig* cfg, SensorData* data, bool* pump)
  : config(cfg), sensorData(data), pumpState(pump) {
  server = new AsyncWebServer(80);

  // Initialize history buffer
  lastWiFiCheck = 0;

  recentHistoryCount = 0;
  recentHistoryIndex = 0;
  lastRecentHistoryUpdate = 0;

  archiveHistoryCount = 0;
  archiveHistoryIndex = 0;
  lastArchiveHistoryUpdate = 0;
}

WebServerManager::~WebServerManager() {
  if (server) {
    delete server;
  }
}

void WebServerManager::begin() {
  // Initialize LittleFS
  if (!LittleFS.begin(true)) {
    logger.error("LittleFS mount failed!");
    logger.warning("Make sure to upload filesystem with: pio run --target uploadfs");
  } else {
    logger.success("LittleFS mounted successfully");
  }

  // Route: Main page
  server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
    this->handleRoot(request);
  });

  // Route: Get sensor data (JSON)
  server->on("/data", HTTP_GET, [this](AsyncWebServerRequest *request){
    this->handleData(request);
  });

  // Route: Get system info (version, build date)
  server->on("/system/info", HTTP_GET, [](AsyncWebServerRequest *request){
    // Calculate firmware usage
    uint32_t sketchSize = ESP.getSketchSize();
    uint32_t sketchSpace = ESP.getFreeSketchSpace();
    uint32_t sketchTotal = sketchSize + sketchSpace;
    float sketchPercent = (float)sketchSize / sketchTotal * 100.0;

    // Calculate filesystem usage
    uint32_t fsTotal = LittleFS.totalBytes();
    uint32_t fsUsed = LittleFS.usedBytes();
    float fsPercent = fsTotal > 0 ? (float)fsUsed / fsTotal * 100.0 : 0;

    // Heap memory information
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t usedHeap = totalHeap - freeHeap;
    float heapPercent = (float)usedHeap / totalHeap * 100.0;
    uint32_t minFreeHeap = ESP.getMinFreeHeap();
    uint32_t maxAllocHeap = ESP.getMaxAllocHeap();

    // History buffer size (constant)
    const uint32_t historyBufferSize = (RECENT_HISTORY_POINTS + ARCHIVE_HISTORY_POINTS) * sizeof(TempDataPoint);

    String json = "{";
    json += "\"version\":\"" + String(FIRMWARE_VERSION) + "\",";
    json += "\"resetReason\":\"" + String(getResetReason()) + "\",";
    json += "\"buildDate\":\"" + String(BUILD_DATE) + "\",";
    json += "\"buildTime\":\"" + String(BUILD_TIME) + "\",";
    json += "\"fsBuildDate\":\"" + String(__DATE__) + "\",";
    json += "\"fsBuildTime\":\"" + String(__TIME__) + "\",";
    json += "\"chipModel\":\"" + String(ESP.getChipModel()) + "\",";
    json += "\"cpuFreq\":" + String(ESP.getCpuFreqMHz()) + ",";
    json += "\"flashSize\":" + String(ESP.getFlashChipSize()) + ",";
    json += "\"freeHeap\":" + String(freeHeap) + ",";
    json += "\"totalHeap\":" + String(totalHeap) + ",";
    json += "\"usedHeap\":" + String(usedHeap) + ",";
    json += "\"heapPercent\":" + String(heapPercent, 1) + ",";
    json += "\"minFreeHeap\":" + String(minFreeHeap) + ",";
    json += "\"maxAllocHeap\":" + String(maxAllocHeap) + ",";
    json += "\"historyBufferSize\":" + String(historyBufferSize) + ",";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"sketchSize\":" + String(sketchSize) + ",";
    json += "\"sketchTotal\":" + String(sketchTotal) + ",";
    json += "\"sketchPercent\":" + String(sketchPercent, 1) + ",";
    json += "\"fsUsed\":" + String(fsUsed) + ",";
    json += "\"fsTotal\":" + String(fsTotal) + ",";
    json += "\"fsPercent\":" + String(fsPercent, 1);
    json += "}";
    request->send(200, "application/json", json);
  });

  // Route: Update temperature configuration
  server->on("/config", HTTP_POST, [this](AsyncWebServerRequest *request){
    this->handleConfig(request);
  });

  // Route: Control pump (manual/auto)
  server->on("/pump", HTTP_POST, [this](AsyncWebServerRequest *request){
    this->handlePump(request);
  });

  // Route: Update WiFi settings
  server->on("/wifi", HTTP_POST, [this](AsyncWebServerRequest *request){
    this->handleWiFi(request);
  });

  // Route: Reset configuration
  server->on("/reset", HTTP_POST, [this](AsyncWebServerRequest *request){
    this->handleReset(request);
  });

  // Route: Restart ESP32
  server->on("/restart", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Restarting ESP32...");
    logger.warning("Restart requested from web interface");
    delay(500);
    ESP.restart();
  });

  // Route: Get logs
  server->on("/logs", HTTP_GET, [this](AsyncWebServerRequest *request){
    String logsJSON = logger.getLogsJSON(100);
    request->send(200, "application/json", logsJSON);
  });

  // New Daily History Routes - Register specific routes before general ones to avoid prefix collisions
  server->on("/history/daily/csv", HTTP_GET, [this](AsyncWebServerRequest *request){
    this->handleDownloadDaily(request);
  });

  server->on("/history/daily", HTTP_GET, [this](AsyncWebServerRequest *request){
    this->handleDailyHistory(request);
  });

  // Route: Get high-resolution temperature history (24h RAM buffer)
  server->on("/history", HTTP_GET, [this](AsyncWebServerRequest *request){
    this->handleHistory(request);
  });

  // Route: Get detected sensors
  server->on("/sensors", HTTP_GET, [this](AsyncWebServerRequest *request){
    this->handleSensors(request);
  });

  server->on("/history3m", HTTP_GET, [](AsyncWebServerRequest *request){
    if (LittleFS.exists("/history3m.html")) {
      request->send(LittleFS, "/history3m.html", "text/html");
    } else {
      request->send(404, "text/plain", "history3m.html missing");
    }
  });

  // Route: Save sensor role mapping
  server->on("/sensors/mapping", HTTP_POST, [this](AsyncWebServerRequest *request){
    this->handleSensorMapping(request);
  });

  // Route: Clear logs
  server->on("/logs/clear", HTTP_POST, [this](AsyncWebServerRequest *request){
    logger.clear();
    request->send(200, "text/plain", "Logs cleared");
  });

  // Route: Start WiFi Scan (async)
  server->on("/wifi/scan/start", HTTP_GET, [](AsyncWebServerRequest *request){
    logger.info("Starting async WiFi scan");
    WiFi.scanNetworks(true, false); // true = async, false = show hidden networks
    request->send(200, "text/plain", "Scan started");
  });

  // Route: Get WiFi Scan Results
  server->on("/wifi/scan", HTTP_GET, [](AsyncWebServerRequest *request){
    int n = WiFi.scanComplete();

    if (n == WIFI_SCAN_RUNNING) {
      request->send(202, "application/json", "{\"status\":\"scanning\"}");
      return;
    }

    if (n == WIFI_SCAN_FAILED) {
      logger.error("WiFi scan failed");
      WiFi.scanDelete();
      request->send(500, "application/json", "{\"status\":\"failed\"}");
      return;
    }

    // Scan completed successfully
    String json = "[";
    for (int i = 0; i < n; i++) {
      if (i > 0) json += ",";
      json += "{";
      json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
      json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
      json += "\"encryption\":" + String(WiFi.encryptionType(i));
      json += "}";
    }
    json += "]";
    logger.infof("WiFi scan completed: %d networks found", n);
    WiFi.scanDelete();
    request->send(200, "application/json", json);
  });

  // Route: OTA Firmware Update
  server->on("/update/firmware", HTTP_POST, [](AsyncWebServerRequest *request){
    bool shouldReboot = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain",
      shouldReboot ? "Firmware mis à jour - Redémarrage..." : "Erreur de mise à jour");
    response->addHeader("Connection", "close");
    request->send(response);
    if (shouldReboot) {
      logger.success("Firmware uploaded successfully - restarting");
      delay(1000);
      ESP.restart();
    } else {
      logger.error("Firmware upload failed");
    }
  }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    if (!index) {
      logger.infof("Firmware update started: %s", filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    }
    if (Update.write(data, len) != len) {
      Update.printError(Serial);
    }
    if (final) {
      if (Update.end(true)) {
        logger.infof("Firmware update completed: %u bytes", index + len);
      } else {
        Update.printError(Serial);
      }
    }
  });

  // Route: OTA Filesystem Update
  server->on("/update/filesystem", HTTP_POST, [](AsyncWebServerRequest *request){
    bool shouldReboot = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain",
      shouldReboot ? "Filesystem mis à jour - Redémarrage..." : "Erreur de mise à jour");
    response->addHeader("Connection", "close");
    request->send(response);
    if (shouldReboot) {
      logger.success("Filesystem uploaded successfully - restarting");
      delay(1000);
      ESP.restart();
    } else {
      logger.error("Filesystem upload failed");
    }
  }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    if (!index) {
      logger.infof("Filesystem update started: %s", filename.c_str());
      // For LittleFS OTA, use U_SPIFFS with the filesystem total size 
      uint32_t fsSize = LittleFS.totalBytes();
      if (!Update.begin(fsSize, U_SPIFFS)) {
        Update.printError(Serial);
      }
    }
    if (Update.write(data, len) != len) {
      Update.printError(Serial);
    }
    if (final) {
      if (Update.end(true)) {
        logger.infof("Filesystem update completed: %u bytes", index + len);
      } else {
        Update.printError(Serial);
      }
    }
  });

  server->begin();
  logger.success("HTTP server started on port 80");
}

void WebServerManager::connectWiFi() {
  Serial.println("\n[Connecting to WiFi]");
  Serial.printf("SSID: %s\n", config->wifi.ssid);
  Serial.printf("Hostname: %s.local\n", config->wifi.hostname);

  logger.infof("Connecting to WiFi: %s", config->wifi.ssid);

  // Register WiFi event handler for connection monitoring
  WiFi.onEvent(onWiFiEvent);

  // Set hostname before connecting
  WiFi.setHostname(config->wifi.hostname);
  WiFi.mode(WIFI_STA);

  // Enable auto-reconnect to handle connection drops
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  WiFi.begin(config->wifi.ssid, config->wifi.password);

  int attempts = 0;
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    esp_task_wdt_reset(); // Pet the watchdog during connection attempts
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    logger.successf("WiFi connected - IP: %s", WiFi.localIP().toString().c_str());
    logger.infof("Signal strength: %d dBm", WiFi.RSSI());

    // Initialize real time now that WiFi is available
    if (initializeTime()) {
      logger.success("Real-time clock synchronized successfully");
    } else {
      logger.warning("Real-time clock synchronization failed, using uptime-based timestamps");
    }
  } else {
    logger.error("WiFi connection failed! Starting Access Point (AP) mode...");
    
    WiFi.mode(WIFI_AP);
    String apSSID = "Spa-" + String(config->wifi.hostname);
    
    // Start AP without password for configuration recovery
    if (WiFi.softAP(apSSID.c_str())) {
      logger.successf("AP Mode started: %s", apSSID.c_str());
      logger.infof("Connect to this network and go to http://192.168.4.1");
    } else {
      logger.error("Failed to start AP Mode");
    }
  }

  // Start mDNS responder (Works in both STA and AP mode)
  if (MDNS.begin(config->wifi.hostname)) {
    logger.successf("mDNS started: http://%s.local", config->wifi.hostname);
    MDNS.addService("http", "tcp", 80);
  } else {
    logger.error("Failed to start mDNS responder");
  }
}

bool WebServerManager::saveDailyStats(float minT, float maxT, float hours) {
    File file = LittleFS.open("/daily_stats.csv", "a"); // Append mode
    if (!file) {
        logger.error("Failed to open /daily_stats.csv for appending");
        return false;
    }
    
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char dateBuf[12];
    strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d", timeinfo);
    
    file.printf("%s,%.1f,%.1f,%.2f\n", dateBuf, minT, maxT, hours);
    file.close();
    return true;
}

void WebServerManager::handleDailyHistory(AsyncWebServerRequest *request) {
    if (!LittleFS.exists("/daily_stats.csv")) {
        request->send(200, "application/json", "{\"points\":[]}");
        return;
    }
    
    File file = LittleFS.open("/daily_stats.csv", "r");
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print("{\"points\":[");
    
    bool first = true;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        if (line.length() < 10) continue;
        
        int c1 = line.indexOf(',');
        int c2 = line.indexOf(',', c1 + 1);
        int c3 = line.indexOf(',', c2 + 1);
        
        if (c1 != -1 && c2 != -1) {
            if (!first) response->print(",");
            response->print("{");
            response->print("\"d\":\"" + line.substring(0, c1) + "\",");
            response->print("\"min\":" + line.substring(c1 + 1, c2) + ",");
            if (c3 != -1) {
                response->print("\"max\":" + line.substring(c2 + 1, c3) + ",");
                response->print("\"c\":" + line.substring(c3 + 1));
            } else {
                response->print("\"max\":" + line.substring(c2 + 1) + ",");
                response->print("\"c\":0");
            }
            response->print("}");
            first = false;
        }
    }
    
    response->print("]}");
    file.close();
    request->send(response);
}

void WebServerManager::handleDownloadDaily(AsyncWebServerRequest *request) {
    if (LittleFS.exists("/daily_stats.csv")) {
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/daily_stats.csv", "text/csv");
        response->addHeader("Content-Disposition", "attachment; filename=daily_stats.csv");
        request->send(response);
    } else {
        request->send(404, "text/plain", "No history available");
    }
}
void WebServerManager::end() {
  server->end();
  Serial.println("HTTP server stopped");
}

void WebServerManager::checkWiFiConnection() {
  unsigned long now = millis();

  // Check WiFi status every 30 seconds
  if (now - lastWiFiCheck < 30000) {
    return;
  }

  lastWiFiCheck = now;

  // If disconnected, attempt reconnection
  if (WiFi.status() != WL_CONNECTED) {
    logger.warning("WiFi disconnected - attempting reconnect");
    WiFi.reconnect();

    // Wait briefly for reconnection (non-blocking with timeout)
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      esp_task_wdt_reset();
      delay(500);
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      logger.successf("WiFi reconnected - IP: %s", WiFi.localIP().toString().c_str());

      // Re-announce mDNS after reconnection
      if (MDNS.begin(config->wifi.hostname)) {
        MDNS.addService("http", "tcp", 80);
      }
    } else {
      logger.error("WiFi reconnection failed - will retry in 30 seconds");
    }
  }
}

void WebServerManager::updateSensorData(float air, float spa, float panel) {
  sensorData->airTemp = air;
  sensorData->spaTemp = spa;
  sensorData->panelTemp = panel;
}

void WebServerManager::updatePumpState(bool state) {
  *pumpState = state;
}

void WebServerManager::handleRoot(AsyncWebServerRequest *request) {
  // Try to serve from LittleFS
  if (LittleFS.exists("/index.html")) {
    request->send(LittleFS, "/index.html", "text/html");
  } else {
    // Fallback error message
    String error = "<!DOCTYPE html><html><head><title>Error</title></head><body>";
    error += "<h1>Error: index.html not found</h1>";
    error += "<p>Please upload the filesystem image using:</p>";
    error += "<pre>pio run --target uploadfs</pre>";
    error += "</body></html>";
    request->send(404, "text/html", error);
    logger.error("/index.html not found in LittleFS!");
  }
}

void WebServerManager::handleData(AsyncWebServerRequest *request) {
  String json = "{";
  json.reserve(512); // Pre-allocate memory to prevent fragmentation

  json += "\"airTemp\":" + String(sensorData->airTemp, 1) + ",";
  json += "\"spaTemp\":" + String(sensorData->spaTemp, 1) + ",";
  json += "\"panelTemp\":" + String(sensorData->panelTemp, 1) + ",";
  json += "\"pumpState\":" + String(*pumpState ? "true" : "false") + ",";
  json += "\"tempDiff\":" + String(config->temp.tempDifferenceThreshold, 1) + ",";
  json += "\"hysteresis\":" + String(config->temp.hysteresis, 1) + ",";
  json += "\"minExternal\":" + String(config->temp.minExternalTemp, 1) + ",";
  json += "\"maxSpa\":" + String(config->temp.maxSpaTemp, 1) + ",";
  json += "\"apMode\":" + String(WiFi.getMode() == WIFI_AP ? "true" : "false") + ",";
  json += "\"manualOverride\":" + String(config->temp.manualOverride ? "true" : "false") + ",";
  json += "\"totalPumpHours\":" + String(config->temp.totalPumpHours, 2) + ",";
  json += "\"dayPumpHours\":" + String(dayPumpHours, 2) + ",";
  json += "\"sampleInterval\":" + String(config->temp.sampleInterval) + ",";
  json += "\"sampleDuration\":" + String(config->temp.sampleDuration) + ",";
  json += "\"wifiSSID\":\"" + String(config->wifi.ssid) + "\",";
  json += "\"wifiPassword\":\"" + String(config->wifi.password) + "\",";
  json += "\"wifiHostname\":\"" + String(config->wifi.hostname) + "\",";
  json += "\"wifiIP\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"wifiRSSI\":" + String(WiFi.RSSI());
  json += "}";
  request->send(200, "application/json", json);
}

void WebServerManager::handleConfig(AsyncWebServerRequest *request) {
  bool updated = false;

  if (request->hasParam("tempDiff", true)) {
    config->temp.tempDifferenceThreshold = request->getParam("tempDiff", true)->value().toFloat();
    updated = true;
  }
  if (request->hasParam("hysteresis", true)) {
    config->temp.hysteresis = request->getParam("hysteresis", true)->value().toFloat();
    updated = true;
  }
  if (request->hasParam("minExternal", true)) {
    config->temp.minExternalTemp = request->getParam("minExternal", true)->value().toFloat();
    updated = true;
  }
  if (request->hasParam("maxSpa", true)) {
    config->temp.maxSpaTemp = request->getParam("maxSpa", true)->value().toFloat();
    updated = true;
  }
  if (request->hasParam("sampleInterval", true)) {
    config->temp.sampleInterval = request->getParam("sampleInterval", true)->value().toInt();
    updated = true;
  }
  if (request->hasParam("sampleDuration", true)) {
    config->temp.sampleDuration = request->getParam("sampleDuration", true)->value().toInt();
    updated = true;
  }

  if (updated) {
    configManager.saveTempParams(config->temp);
    logger.infof("Temperature parameters updated: diff=%.1f, hyst=%.1f, min=%.1f, max=%.1f, interval=%d, duration=%d",
                 config->temp.tempDifferenceThreshold,
                 config->temp.hysteresis,
                 config->temp.minExternalTemp,
                 config->temp.maxSpaTemp,
                 config->temp.sampleInterval,
                 config->temp.sampleDuration);
    request->send(200, "text/plain", "Settings saved to JSON");
  } else {
    request->send(400, "text/plain", "No parameters provided");
  }
}

void WebServerManager::handlePump(AsyncWebServerRequest *request) {
  if (request->hasParam("manual", true)) {
    String manual = request->getParam("manual", true)->value();

    if (manual == "on") {
      config->temp.manualOverride = true;
      config->temp.pumpState = true;
      setRelay(true);
      logger.info("Pump: MANUAL ON");
    } else if (manual == "off") {
      config->temp.manualOverride = true;
      config->temp.pumpState = false;
      setRelay(false);
      logger.info("Pump: MANUAL OFF");
    } else if (manual == "auto") {
      config->temp.manualOverride = false;
      logger.info("Pump: AUTO MODE");
    }

    configManager.savePumpState(config->temp);
    request->send(200, "text/plain", "Pump control updated");
  } else {
    request->send(400, "text/plain", "No command provided");
  }
}

void WebServerManager::handleWiFi(AsyncWebServerRequest *request) {
  bool updated = false;

  if (request->hasParam("ssid", true)) {
    String ssid = request->getParam("ssid", true)->value();
    strncpy(config->wifi.ssid, ssid.c_str(), sizeof(config->wifi.ssid) - 1);
    config->wifi.ssid[sizeof(config->wifi.ssid) - 1] = '\0';
    updated = true;
  }
  if (request->hasParam("password", true)) {
    String password = request->getParam("password", true)->value();
    strncpy(config->wifi.password, password.c_str(), sizeof(config->wifi.password) - 1);
    config->wifi.password[sizeof(config->wifi.password) - 1] = '\0';
    updated = true;
  }

  if (updated) {
    configManager.saveWiFiConfig(config->wifi);
    logger.warning("WiFi credentials updated - restart required");
    request->send(200, "text/plain", "WiFi settings saved to JSON - restart ESP32 to apply");
  } else {
    request->send(400, "text/plain", "No WiFi parameters provided");
  }
}

void WebServerManager::handleReset(AsyncWebServerRequest *request) {
  configManager.reset();
  SpaConfig defaultConfig;
  *config = defaultConfig;
  logger.warning("Configuration reset to defaults");
  request->send(200, "text/plain", "Configuration reset - restart recommended");
}

void WebServerManager::recordHistory() {
  unsigned long currentMillis = millis();

  // Record into recent history (1-minute interval)
  if (currentMillis - lastRecentHistoryUpdate >= RECENT_HISTORY_INTERVAL_MS) {
    lastRecentHistoryUpdate = currentMillis;

    recentHistoryBuffer[recentHistoryIndex].timestamp = (unsigned long)(getEpochMillis() / 1000);
    recentHistoryBuffer[recentHistoryIndex].airTemp = sensorData->airTemp;
    recentHistoryBuffer[recentHistoryIndex].spaTemp = sensorData->spaTemp;
    recentHistoryBuffer[recentHistoryIndex].panelTemp = sensorData->panelTemp;
    recentHistoryBuffer[recentHistoryIndex].pumpState = *pumpState;

    recentHistoryIndex = (recentHistoryIndex + 1) % RECENT_HISTORY_POINTS;
    if (recentHistoryCount < RECENT_HISTORY_POINTS) {
      recentHistoryCount++;
    }
  }

  // Record into archive history (30-minute interval)
  if (currentMillis - lastArchiveHistoryUpdate >= ARCHIVE_HISTORY_INTERVAL_MS) {
    lastArchiveHistoryUpdate = currentMillis;

    archiveHistoryBuffer[archiveHistoryIndex].timestamp = (unsigned long)(getEpochMillis() / 1000);
    archiveHistoryBuffer[archiveHistoryIndex].airTemp = sensorData->airTemp;
    archiveHistoryBuffer[archiveHistoryIndex].spaTemp = sensorData->spaTemp;
    archiveHistoryBuffer[archiveHistoryIndex].panelTemp = sensorData->panelTemp;
    archiveHistoryBuffer[archiveHistoryIndex].pumpState = *pumpState;

    archiveHistoryIndex = (archiveHistoryIndex + 1) % ARCHIVE_HISTORY_POINTS;
    if (archiveHistoryCount < ARCHIVE_HISTORY_POINTS) {
      archiveHistoryCount++;
    }
  }
}

void WebServerManager::handleHistory(AsyncWebServerRequest *request) {
  // Using ResponseStream prevents allocating a massive String on the heap,
  // which is the main cause of reboots when generating JSON.
  AsyncResponseStream *response = request->beginResponseStream("application/json");
  response->print("{\"points\":[");
  
  std::vector<TempDataPoint> combinedPoints;
  combinedPoints.reserve(recentHistoryCount + archiveHistoryCount); // Pre-allocate memory

  // 1. Determine the oldest point in the high-res buffer
  unsigned long oldestRecentTimestamp = 0;
  if (recentHistoryCount > 0) {
    int oldestIdx = (recentHistoryIndex + (RECENT_HISTORY_POINTS - recentHistoryCount)) % RECENT_HISTORY_POINTS;
    oldestRecentTimestamp = recentHistoryBuffer[oldestIdx].timestamp;
  }

  // 2. Add archive points (30-min resolution)
  // We only add archive points that are older than our high-res window
  int archiveStartIdx = (archiveHistoryCount < ARCHIVE_HISTORY_POINTS) ? 0 : archiveHistoryIndex;
  for (int i = 0; i < archiveHistoryCount; i++) {
    int idx = (archiveStartIdx + i) % ARCHIVE_HISTORY_POINTS;
    if (oldestRecentTimestamp == 0 || archiveHistoryBuffer[idx].timestamp < oldestRecentTimestamp) {
      combinedPoints.push_back(archiveHistoryBuffer[idx]);
    }
  }

  // 3. Add recent points (1-min resolution)
  int recentStartIdx = (recentHistoryCount < RECENT_HISTORY_POINTS) ? 0 : recentHistoryIndex;
  for (int i = 0; i < recentHistoryCount; i++) {
    int idx = (recentStartIdx + i) % RECENT_HISTORY_POINTS;
    combinedPoints.push_back(recentHistoryBuffer[idx]);
  }

  // Sort all combined points by timestamp to ensure chronological order
  std::sort(combinedPoints.begin(), combinedPoints.end(), [](const TempDataPoint& a, const TempDataPoint& b) {
    return a.timestamp < b.timestamp;
  });

  // Output the combined and sorted points
  for (size_t i = 0; i < combinedPoints.size(); i++) {
    if (i > 0) response->print(",");

    response->print("{");
    response->print("\"t\":");
    response->print((unsigned long long)combinedPoints[i].timestamp * 1000ULL);
    response->print(",");
    response->print("\"a\":"); response->print(combinedPoints[i].airTemp, 1);
    response->print(",\"s\":"); response->print(combinedPoints[i].spaTemp, 1);
    response->print(",\"p\":"); response->print(combinedPoints[i].panelTemp, 1);
    response->print(",\"pump\":"); response->print(combinedPoints[i].pumpState ? "1" : "0");
    response->print("}");
    
    // Yield to the system every 50 points to prevent Watchdog reboots
    if (i % 50 == 0) yield(); 
  }

  response->print("],\"count\":");
  response->print(combinedPoints.size());
  response->print("}");
  request->send(response);
}

void WebServerManager::handleSensorMapping(AsyncWebServerRequest *request) {
  bool updated = false;

  // Parse sensor mapping from POST parameters
  // Expected format: airAddr=XX-XX-XX-XX-XX-XX-XX-XX&spaAddr=...&panelAddr=...&airOffset=...&spaOffset=...&panelOffset=...
  if (request->hasParam("airAddr", true) &&
      request->hasParam("spaAddr", true) &&
      request->hasParam("panelAddr", true)) {

    String airAddr = request->getParam("airAddr", true)->value();
    String spaAddr = request->getParam("spaAddr", true)->value();
    String panelAddr = request->getParam("panelAddr", true)->value();

    // Parse hex addresses
    if (parseAddress(airAddr, config->sensors.airSensorAddress) &&
        parseAddress(spaAddr, config->sensors.spaSensorAddress) &&
        parseAddress(panelAddr, config->sensors.panelSensorAddress)) {

      // Parse calibration offsets if provided
      if (request->hasParam("airOffset", true)) {
        config->sensors.airOffset = request->getParam("airOffset", true)->value().toFloat();
      }
      if (request->hasParam("spaOffset", true)) {
        config->sensors.spaOffset = request->getParam("spaOffset", true)->value().toFloat();
      }
      if (request->hasParam("panelOffset", true)) {
        config->sensors.panelOffset = request->getParam("panelOffset", true)->value().toFloat();
      }

      config->sensors.useMapping = true;
      configManager.saveSensorMapping(config->sensors);

      logger.success("Sensor role mapping and calibration offsets updated");
      request->send(200, "text/plain", "Sensor mapping saved - restart required");
      updated = true;
    }
  }

  if (!updated) {
    request->send(400, "text/plain", "Invalid sensor mapping parameters");
  }
}

// Helper function to parse hex address string like "28-FF-AA-BB-CC-DD-EE-01"
bool parseAddress(const String& addrStr, uint8_t* addr) {
  int byteIndex = 0;
  int strIndex = 0;

  while (byteIndex < 8 && strIndex < addrStr.length()) {
    // Skip separators
    if (addrStr[strIndex] == '-' || addrStr[strIndex] == ':' || addrStr[strIndex] == ' ') {
      strIndex++;
      continue;
    }

    // Read two hex characters
    if (strIndex + 1 < addrStr.length()) {
      String byteStr = addrStr.substring(strIndex, strIndex + 2);
      addr[byteIndex] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
      byteIndex++;
      strIndex += 2;
    } else {
      return false;
    }
  }

  return byteIndex == 8;
}

void WebServerManager::handleSensors(AsyncWebServerRequest *request) {
  String json = "{\"sensors\":[";

  for (int i = 0; i < sensorCount; i++) {
    uint8_t addr[8];
    sensors.getAddress(addr, i);

    if (i > 0) json += ",";
    json += "{\"index\":" + String(i) + ",";
    json += "\"address\":\"";
    for (int j = 0; j < 8; j++) {
      if (j > 0) json += "-";
      if (addr[j] < 16) json += "0";
      json += String(addr[j], HEX);
    }
    json += "\",";

    // Determine current role and offset
    String role = "unassigned";
    float offset = 0.0;
    if (memcmp(addr, airSensor, 8) == 0) {
      role = "air";
      offset = config->sensors.airOffset;
    } else if (memcmp(addr, spaSensor, 8) == 0) {
      role = "spa";
      offset = config->sensors.spaOffset;
    } else if (memcmp(addr, panelSensor, 8) == 0) {
      role = "panel";
      offset = config->sensors.panelOffset;
    }

    json += "\"role\":\"" + role + "\",";
    json += "\"offset\":" + String(offset, 1) + ",";

    // Get current temperature
    float temp = sensors.getTempC(addr);
    json += "\"temp\":" + String(temp, 1);
    json += "}";
  }

  json += "],\"useMapping\":" + String(config->sensors.useMapping ? "true" : "false");
  json += ",\"count\":" + String(sensorCount);
  json += "}";

  request->send(200, "application/json", json);
}
