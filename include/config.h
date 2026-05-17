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
  float hysteresis;               // Hysteresis to turn off pump
  float minExternalTemp;          // Minimum external air temp to allow pump activation
  float maxSpaTemp;               // Maximum spa temp (safety limit)
  bool manualOverride;            // Manual pump control
  bool pumpState;                 // Current pump state (for manual mode persistence)
  int sampleInterval;             // Frequency of pump activation for sampling (minutes)
  int sampleDuration;             // Duration of pump activation for sampling (seconds)
  float totalPumpHours;           // Cumulative runtime in hours

  // Constructor with default values
  TempConfig() {
    tempDifferenceThreshold = 5.0;
    hysteresis = 1.0;
    minExternalTemp = 25.0;
    maxSpaTemp = 40.0;
    manualOverride = false;
    pumpState = false;
    sampleInterval = 60;
    sampleDuration = 120;
    totalPumpHours = 0.0;
  }
};

// WiFi configuration structure
struct WiFiConfig {
  char ssid[64];
  char password[64];
  char hostname[32];

  // Constructor with default values
  WiFiConfig() {
    strncpy(ssid, "YOUR_WIFI_SSID", sizeof(ssid) - 1);
    ssid[sizeof(ssid) - 1] = '\0';
    strncpy(password, "YOUR_WIFI_PASSWORD", sizeof(password) - 1);
    password[sizeof(password) - 1] = '\0';
    strncpy(hostname, "chauffeSpa", sizeof(hostname) - 1);
    hostname[sizeof(hostname) - 1] = '\0';
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

  // Initialize filesystem and load all configurations
  bool begin(SpaConfig& config);

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
