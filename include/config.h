#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Sensor configuration structure
struct SensorMapping {
  uint8_t airSensorAddress[8];
  uint8_t spaSensorAddress[8];
  uint8_t panelSensorAddress[8];
  bool useMapping;  // If false, use default bus index (0=air, 1=spa, 2=panel)
  
  // Calibration offsets for temperature compensation
  float airOffset;    // Offset to add to air sensor reading
  float spaOffset;    // Offset to add to spa sensor reading
  float panelOffset;  // Offset to add to panel sensor reading

  // Constructor with default values
  SensorMapping() {
    memset(airSensorAddress, 0, 8);
    memset(spaSensorAddress, 0, 8);
    memset(panelSensorAddress, 0, 8);
    useMapping = false;
    airOffset = 0.0;
    spaOffset = 0.0;
    panelOffset = 0.0;
  }
};

// Temperature configuration structure
struct TempConfig {
  float tempDifferenceThreshold;  // Degrees C difference to activate pump
  float minPanelTemp;             // Minimum panel temp to activate pump
  float maxSpaTemp;               // Maximum spa temp (safety limit)
  bool manualOverride;            // Manual pump control
  bool pumpState;                 // Current pump state (for manual mode persistence)

  // Constructor with default values
  TempConfig() {
    tempDifferenceThreshold = 5.0;
    minPanelTemp = 25.0;
    maxSpaTemp = 40.0;
    manualOverride = false;
    pumpState = false;
  }
};

// WiFi configuration structure
struct WiFiConfig {
  char ssid[64];
  char password[64];
  char hostname[32];

  // Constructor with default values
  WiFiConfig() {
    strcpy(ssid, "YOUR_WIFI_SSID");
    strcpy(password, "YOUR_WIFI_PASSWORD");
    strcpy(hostname, "chauffeSpa");
  }
};

// Complete system configuration
struct SpaConfig {
  TempConfig temp;
  WiFiConfig wifi;
  SensorMapping sensors;
};

class ConfigManager {
private:
  static const char* TEMP_CONFIG_FILE;
  static const char* WIFI_CONFIG_FILE;
  static const char* SENSOR_CONFIG_FILE;

public:
  ConfigManager();

  // Load temperature configuration from JSON file
  bool loadTempConfig(TempConfig& config);

  // Load WiFi configuration from JSON file
  bool loadWiFiConfig(WiFiConfig& config);

  // Load complete configuration
  bool loadAll(SpaConfig& config);

  // Save temperature configuration to JSON file
  bool saveTempConfig(const TempConfig& config);

  // Save WiFi configuration to JSON file
  bool saveWiFiConfig(const WiFiConfig& config);

  // Save complete configuration
  bool saveAll(const SpaConfig& config);

  // Save only temperature parameters (not pump state)
  bool saveTempParams(const TempConfig& config);

  // Save only pump state
  bool savePumpState(const TempConfig& config);

  // Save sensor mapping configuration to JSON file
  bool saveSensorMapping(const SensorMapping& config);

  // Load sensor mapping configuration from JSON file
  bool loadSensorMapping(SensorMapping& config);

  // Reset to default values
  bool reset();

  // Print configuration to serial
  void printConfig(const SpaConfig& config);
};

#endif // CONFIG_H
