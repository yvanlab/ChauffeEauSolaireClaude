#include "webserver.h"
#include "config.h"
#include "logger.h"
#include <LittleFS.h>

// External configuration manager (defined in main.cpp)
extern ConfigManager configManager;

WebServerManager::WebServerManager(SpaConfig* cfg, SensorData* data, bool* pump)
  : config(cfg), sensorData(data), pumpState(pump) {
  server = new AsyncWebServer(80);
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

  // Route: Get logs
  server->on("/logs", HTTP_GET, [this](AsyncWebServerRequest *request){
    String logsJSON = logger.getLogsJSON(100);
    request->send(200, "application/json", logsJSON);
  });

  // Route: Clear logs
  server->on("/logs/clear", HTTP_POST, [this](AsyncWebServerRequest *request){
    logger.clear();
    request->send(200, "text/plain", "Logs cleared");
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
  json += "\"manualOverride\":" + String(config->temp.manualOverride ? "true" : "false");
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
      *pumpState = true;
      logger.info("Pump: MANUAL ON");
    } else if (manual == "off") {
      config->temp.manualOverride = true;
      config->temp.pumpState = false;
      *pumpState = false;
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
