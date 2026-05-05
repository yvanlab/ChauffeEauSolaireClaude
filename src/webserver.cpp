#include "webserver.h"
#include "config.h"
#include "logger.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <Update.h>

// Forward declarations
bool parseAddress(const String& addrStr, uint8_t* addr);

// External sensor variables (defined in main.cpp)
extern DallasTemperature sensors;
extern int sensorCount;
extern uint8_t airSensor[8], spaSensor[8], panelSensor[8];

// External configuration manager (defined in main.cpp)
extern ConfigManager configManager;
extern void setRelay(bool on);

// External version info (defined in main.cpp)
extern const char* FIRMWARE_VERSION;
extern const char* BUILD_DATE;
extern const char* BUILD_TIME;

// External version info (defined in main.cpp)
extern const char* FIRMWARE_VERSION;
extern const char* BUILD_DATE;
extern const char* BUILD_TIME;

WebServerManager::WebServerManager(SpaConfig* cfg, SensorData* data, bool* pump)
  : config(cfg), sensorData(data), pumpState(pump) {
  server = new AsyncWebServer(80);

  // Initialize history buffer
  historyBuffer = new TempDataPoint[MAX_HISTORY_POINTS];
  historyCount = 0;
  historyIndex = 0;
  lastHistoryUpdate = 0;
}

WebServerManager::~WebServerManager() {
  if (server) {
    delete server;
  }
  if (historyBuffer) {
    delete[] historyBuffer;
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
    const uint32_t historyBufferSize = MAX_HISTORY_POINTS * sizeof(TempDataPoint);

    String json = "{";
    json += "\"version\":\"" + String(FIRMWARE_VERSION) + "\",";
    json += "\"buildDate\":\"" + String(BUILD_DATE) + "\",";
    json += "\"buildTime\":\"" + String(BUILD_TIME) + "\",";
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

  // Route: Get temperature history
  server->on("/history", HTTP_GET, [this](AsyncWebServerRequest *request){
    this->handleHistory(request);
  });

  // Route: Get detected sensors
  server->on("/sensors", HTTP_GET, [this](AsyncWebServerRequest *request){
    this->handleSensors(request);
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

void WebServerManager::end() {
  server->end();
  Serial.println("HTTP server stopped");
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
  json += "\"airTemp\":" + String(sensorData->airTemp, 1) + ",";
  json += "\"spaTemp\":" + String(sensorData->spaTemp, 1) + ",";
  json += "\"panelTemp\":" + String(sensorData->panelTemp, 1) + ",";
  json += "\"pumpState\":" + String(*pumpState ? "true" : "false") + ",";
  json += "\"tempDiff\":" + String(config->temp.tempDifferenceThreshold, 1) + ",";
  json += "\"minPanel\":" + String(config->temp.minPanelTemp, 1) + ",";
  json += "\"maxSpa\":" + String(config->temp.maxSpaTemp, 1) + ",";
  json += "\"manualOverride\":" + String(config->temp.manualOverride ? "true" : "false") + ",";
  json += "\"wifiSSID\":\"" + String(config->wifi.ssid) + "\",";
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
  if (request->hasParam("minPanel", true)) {
    config->temp.minPanelTemp = request->getParam("minPanel", true)->value().toFloat();
    updated = true;
  }
  if (request->hasParam("maxSpa", true)) {
    config->temp.maxSpaTemp = request->getParam("maxSpa", true)->value().toFloat();
    updated = true;
  }

  if (updated) {
    configManager.saveTempParams(config->temp);
    logger.infof("Temperature parameters updated: diff=%.1f, min=%.1f, max=%.1f",
                 config->temp.tempDifferenceThreshold,
                 config->temp.minPanelTemp,
                 config->temp.maxSpaTemp);
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

  // Only record if enough time has passed
  if (currentMillis - lastHistoryUpdate < HISTORY_INTERVAL_MS) {
    return;
  }

  lastHistoryUpdate = currentMillis;

  // Store current temperatures and pump state in circular buffer
  historyBuffer[historyIndex].timestamp = currentMillis;
  historyBuffer[historyIndex].airTemp = sensorData->airTemp;
  historyBuffer[historyIndex].spaTemp = sensorData->spaTemp;
  historyBuffer[historyIndex].panelTemp = sensorData->panelTemp;
  historyBuffer[historyIndex].pumpState = *pumpState;

  // Move to next position in circular buffer
  historyIndex = (historyIndex + 1) % MAX_HISTORY_POINTS;

  // Track how many points we have (up to MAX_HISTORY_POINTS)
  if (historyCount < MAX_HISTORY_POINTS) {
    historyCount++;
  }
}

void WebServerManager::handleHistory(AsyncWebServerRequest *request) {
  String json = "{\"points\":[";

  // Read from circular buffer in chronological order
  int startIdx = (historyCount < MAX_HISTORY_POINTS) ? 0 : historyIndex;

  for (int i = 0; i < historyCount; i++) {
    int idx = (startIdx + i) % MAX_HISTORY_POINTS;

    if (i > 0) json += ",";
    json += "{";
    json += "\"t\":" + String(historyBuffer[idx].timestamp) + ",";
    json += "\"a\":" + String(historyBuffer[idx].airTemp, 1) + ",";
    json += "\"s\":" + String(historyBuffer[idx].spaTemp, 1) + ",";
    json += "\"p\":" + String(historyBuffer[idx].panelTemp, 1) + ",";
    json += "\"pump\":" + String(historyBuffer[idx].pumpState ? "1" : "0");
    json += "}";
  }

  json += "],\"count\":" + String(historyCount) + "}";
  request->send(200, "application/json", json);
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
