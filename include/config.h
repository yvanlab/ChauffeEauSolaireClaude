#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

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
};

class ConfigManager {
private:
  static const char* TEMP_CONFIG_FILE;
  static const char* WIFI_CONFIG_FILE;

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

  // Reset to default values
  bool reset();

  // Print configuration to serial
  void printConfig(const SpaConfig& config);
};

#endif // CONFIG_H
