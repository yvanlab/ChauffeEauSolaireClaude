#include "config.h"
#include "logger.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

const char* ConfigManager::TEMP_CONFIG_FILE = "/temp_config.json";
const char* ConfigManager::WIFI_CONFIG_FILE = "/wifi_config.json";
const char* ConfigManager::SENSOR_CONFIG_FILE = "/sensor_config.json";

ConfigManager::ConfigManager() {
}

bool ConfigManager::loadTempConfig(TempConfig& config) {
  if (!LittleFS.exists(TEMP_CONFIG_FILE)) {
    logger.warningf("Temperature config file not found: %s", TEMP_CONFIG_FILE);
    logger.info("Using default temperature values");
    return false;
  }

  File file = LittleFS.open(TEMP_CONFIG_FILE, "r");
  if (!file) {
    logger.error("Failed to open temperature config file");
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    logger.errorf("Failed to parse temperature config: %s", error.c_str());
    return false;
  }

  // Load values from JSON
  config.tempDifferenceThreshold = doc["tempDifferenceThreshold"] | 5.0;
  config.hysteresis = doc["hysteresis"] | 1.0;
  config.minExternalTemp = doc["minExternalTemp"] | 25.0;
  config.maxSpaTemp = doc["maxSpaTemp"] | 40.0;
  config.manualOverride = doc["manualOverride"] | false;
  config.pumpState = doc["pumpState"] | false;
  config.sampleInterval = doc["sampleInterval"] | 60;
  config.sampleDuration = doc["sampleDuration"] | 4;

  logger.success("Temperature configuration loaded from JSON");
  return true;
}

bool ConfigManager::loadWiFiConfig(WiFiConfig& config) {
  if (!LittleFS.exists(WIFI_CONFIG_FILE)) {
    logger.warningf("WiFi config file not found: %s", WIFI_CONFIG_FILE);
    logger.info("Using default WiFi values");
    return false;
  }

  File file = LittleFS.open(WIFI_CONFIG_FILE, "r");
  if (!file) {
    logger.error("Failed to open WiFi config file");
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    logger.errorf("Failed to parse WiFi config: %s", error.c_str());
    return false;
  }

  // Load values from JSON
  const char* ssid = doc["ssid"] | "YOUR_WIFI_SSID";
  const char* password = doc["password"] | "YOUR_WIFI_PASSWORD";
  const char* hostname = doc["hostname"] | "chauffeSpa";

  strncpy(config.ssid, ssid, sizeof(config.ssid) - 1);
  config.ssid[sizeof(config.ssid) - 1] = '\0';

  strncpy(config.password, password, sizeof(config.password) - 1);
  config.password[sizeof(config.password) - 1] = '\0';

  strncpy(config.hostname, hostname, sizeof(config.hostname) - 1);
  config.hostname[sizeof(config.hostname) - 1] = '\0';

  logger.success("WiFi configuration loaded from JSON");
  return true;
}

bool ConfigManager::loadSensorMapping(SensorMapping& config) {
  if (!LittleFS.exists(SENSOR_CONFIG_FILE)) {
    logger.warningf("Sensor mapping file not found: %s", SENSOR_CONFIG_FILE);
    logger.info("Using default sensor order (0=air, 1=spa, 2=panel)");
    return false;
  }

  File file = LittleFS.open(SENSOR_CONFIG_FILE, "r");
  if (!file) {
    logger.error("Failed to open sensor mapping file");
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    logger.errorf("Failed to parse sensor mapping: %s", error.c_str());
    return false;
  }

  // Load useMapping flag
  config.useMapping = doc["useMapping"] | false;

  // Load calibration offsets
  config.airOffset = doc["airOffset"] | 0.0;
  config.spaOffset = doc["spaOffset"] | 0.0;
  config.panelOffset = doc["panelOffset"] | 0.0;

  if (config.useMapping) {
    // Load sensor addresses from JSON (stored as hex string arrays)
    JsonArray airAddr = doc["airSensor"];
    JsonArray spaAddr = doc["spaSensor"];
    JsonArray panelAddr = doc["panelSensor"];

    if (airAddr.size() == 8 && spaAddr.size() == 8 && panelAddr.size() == 8) {
      for (int i = 0; i < 8; i++) {
        config.airSensorAddress[i] = airAddr[i];
        config.spaSensorAddress[i] = spaAddr[i];
        config.panelSensorAddress[i] = panelAddr[i];
      }
      logger.success("Sensor mapping loaded from JSON");
    } else {
      logger.error("Invalid sensor addresses in config");
      config.useMapping = false;
      return false;
    }
  }

  return true;
}

bool ConfigManager::loadAll(SpaConfig& config) {
  bool tempOk = loadTempConfig(config.temp);
  bool wifiOk = loadWiFiConfig(config.wifi);
  bool sensorOk = loadSensorMapping(config.sensors);
  return tempOk && wifiOk;
}

bool ConfigManager::saveTempConfig(const TempConfig& config) {
  JsonDocument doc;

  doc["tempDifferenceThreshold"] = config.tempDifferenceThreshold;
  doc["hysteresis"] = config.hysteresis;
  doc["minExternalTemp"] = config.minExternalTemp;
  doc["maxSpaTemp"] = config.maxSpaTemp;
  doc["manualOverride"] = config.manualOverride;
  doc["pumpState"] = config.pumpState;
  doc["sampleInterval"] = config.sampleInterval;
  doc["sampleDuration"] = config.sampleDuration;

  File file = LittleFS.open(TEMP_CONFIG_FILE, "w");
  if (!file) {
    Serial.println("✗ Failed to open temperature config for writing");
    return false;
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println("✗ Failed to write temperature config");
    file.close();
    return false;
  }

  file.close();
  logger.success("Temperature configuration saved to JSON");
  return true;
}

bool ConfigManager::saveWiFiConfig(const WiFiConfig& config) {
  JsonDocument doc;

  doc["ssid"] = config.ssid;
  doc["password"] = config.password;
  doc["hostname"] = config.hostname;

  File file = LittleFS.open(WIFI_CONFIG_FILE, "w");
  if (!file) {
    Serial.println("✗ Failed to open WiFi config for writing");
    return false;
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println("✗ Failed to write WiFi config");
    file.close();
    return false;
  }

  file.close();
  logger.success("WiFi configuration saved to JSON");
  return true;
}

bool ConfigManager::saveAll(const SpaConfig& config) {
  bool tempOk = saveTempConfig(config.temp);
  bool wifiOk = saveWiFiConfig(config.wifi);
  return tempOk && wifiOk;
}

bool ConfigManager::saveTempParams(const TempConfig& config) {
  // Save only temperature parameters, preserve pump state
  TempConfig current;
  loadTempConfig(current);  // Load current to get pump state

  JsonDocument doc;
  doc["tempDifferenceThreshold"] = config.tempDifferenceThreshold;
  doc["hysteresis"] = config.hysteresis;
  doc["minExternalTemp"] = config.minExternalTemp;
  doc["maxSpaTemp"] = config.maxSpaTemp;
  doc["manualOverride"] = current.manualOverride;  // Keep current
  doc["pumpState"] = current.pumpState;            // Keep current
  doc["sampleInterval"] = config.sampleInterval;
  doc["sampleDuration"] = config.sampleDuration;

  File file = LittleFS.open(TEMP_CONFIG_FILE, "w");
  if (!file) {
    Serial.println("✗ Failed to open temperature config for writing");
    return false;
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println("✗ Failed to write temperature parameters");
    file.close();
    return false;
  }

  file.close();
  logger.success("Temperature parameters saved");
  return true;
}

bool ConfigManager::savePumpState(const TempConfig& config) {
  // Save only pump state, preserve temperature parameters
  TempConfig current;
  loadTempConfig(current);  // Load current to get temp params

  JsonDocument doc;
  doc["tempDifferenceThreshold"] = current.tempDifferenceThreshold;
  doc["hysteresis"] = current.hysteresis;
  doc["minExternalTemp"] = current.minExternalTemp;
  doc["maxSpaTemp"] = current.maxSpaTemp;
  doc["manualOverride"] = config.manualOverride;
  doc["pumpState"] = config.pumpState;
  doc["sampleInterval"] = current.sampleInterval;
  doc["sampleDuration"] = current.sampleDuration;

  File file = LittleFS.open(TEMP_CONFIG_FILE, "w");
  if (!file) {
    Serial.println("✗ Failed to open temperature config for writing");
    return false;
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println("✗ Failed to write pump state");
    file.close();
    return false;
  }

  file.close();
  logger.success("Pump state saved");
  return true;
}

bool ConfigManager::saveSensorMapping(const SensorMapping& config) {
  JsonDocument doc;

  doc["useMapping"] = config.useMapping;

  // Save sensor addresses as arrays
  JsonArray airAddr = doc["airSensor"].to<JsonArray>();
  JsonArray spaAddr = doc["spaSensor"].to<JsonArray>();
  JsonArray panelAddr = doc["panelSensor"].to<JsonArray>();

  for (int i = 0; i < 8; i++) {
    airAddr.add(config.airSensorAddress[i]);
    spaAddr.add(config.spaSensorAddress[i]);
    panelAddr.add(config.panelSensorAddress[i]);
  }

  // Save calibration offsets
  doc["airOffset"] = config.airOffset;
  doc["spaOffset"] = config.spaOffset;
  doc["panelOffset"] = config.panelOffset;

  File file = LittleFS.open(SENSOR_CONFIG_FILE, "w");
  if (!file) {
    Serial.println("✗ Failed to open sensor mapping for writing");
    return false;
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println("✗ Failed to write sensor mapping");
    file.close();
    return false;
  }

  file.close();
  logger.success("Sensor mapping saved to JSON");
  return true;
}

bool ConfigManager::reset() {
  // Delete both config files
  bool tempDeleted = false;
  bool wifiDeleted = false;

  if (LittleFS.exists(TEMP_CONFIG_FILE)) {
    tempDeleted = LittleFS.remove(TEMP_CONFIG_FILE);
  }

  if (LittleFS.exists(WIFI_CONFIG_FILE)) {
    wifiDeleted = LittleFS.remove(WIFI_CONFIG_FILE);
  }

  // Recreate with defaults
  TempConfig defaultTemp;
  WiFiConfig defaultWiFi;

  saveTempConfig(defaultTemp);
  saveWiFiConfig(defaultWiFi);

  logger.warning("Configuration reset to defaults");
  return true;
}

void ConfigManager::printConfig(const SpaConfig& config) {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║      Current Configuration             ║");
  Serial.println("╚════════════════════════════════════════╝");

  Serial.println("\n[Temperature Parameters]");
  Serial.printf("  File: %s\n", TEMP_CONFIG_FILE);
  Serial.printf("  Temp difference threshold: %.1f°C\n", config.temp.tempDifferenceThreshold);
  Serial.printf("  Min external temp: %.1f°C\n", config.temp.minExternalTemp);
  Serial.printf("  Max spa temp: %.1f°C\n", config.temp.maxSpaTemp);
  Serial.printf("  Sample Interval: %d mins\n", config.temp.sampleInterval);
  Serial.printf("  Sample Duration: %d secs\n", config.temp.sampleDuration);

  Serial.println("\n[WiFi Settings]");
  Serial.printf("  File: %s\n", WIFI_CONFIG_FILE);
  Serial.printf("  SSID: %s\n", config.wifi.ssid);
  Serial.printf("  Password: %s\n", strlen(config.wifi.password) > 0 ? "****" : "(empty)");
  Serial.printf("  Hostname: %s.local\n", config.wifi.hostname);

  Serial.println("\n[Pump State]");
  Serial.printf("  Manual override: %s\n", config.temp.manualOverride ? "YES" : "NO");
  Serial.printf("  Pump state: %s\n", config.temp.pumpState ? "ON" : "OFF");

  Serial.println("════════════════════════════════════════\n");
}
