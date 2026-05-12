#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LittleFS.h>
#include "config.h"
#include "webserver.h"
#include "logger.h"
#include "time_utils.h"
#include <esp_task_wdt.h>

/*
GPIO23	Status LED
GPIO16	Relay #1
GPIO13	Relay #2
GPIO25	Relay #3
GPIO26	Relay #4
*/
// Version information
const char* FIRMWARE_VERSION = "2.4";
const char* BUILD_DATE = __DATE__;
const char* BUILD_TIME = __TIME__;

// Pin definitions
#define ONE_WIRE_BUS 4    // GPIO4 for DS18B20 sensors
#define RELAY_PIN 16      // GPIO14 for pump relay

// Temperature sensors
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Sensor addresses (will be auto-detected)
DeviceAddress airSensor, spaSensor, panelSensor;
int sensorCount = 0;

// Configuration
SpaConfig config;
ConfigManager configManager;

// Sensor data
SensorData sensorData;

// Pump state
bool pumpState = false;

// Web server
WebServerManager* webServer = nullptr;

// Function prototypes
void setupSensors();
void readTemperatures();
void controlPump();
void setRelay(bool on);
void printWelcome();

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize Task Watchdog Timer (10 seconds timeout)
  // Compatibility for both older and newer ESP32 Arduino cores
  #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    esp_task_wdt_config_t wdt_config = { .timeout_ms = 10000, .idle_core_mask = 0, .trigger_panic = true };
    esp_task_wdt_init(&wdt_config);
  #else
    esp_task_wdt_init(10, true);
  #endif
  esp_task_wdt_add(NULL);

  printWelcome();


  // Initialize pins
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Start with pump off

  // Mount LittleFS first (required for loading config files)
  if (!LittleFS.begin(true)) {
    logger.error("LittleFS mount failed!");
  } else {
    logger.success("LittleFS mounted successfully");
  }

  // Load configuration from JSON files
  Serial.println("\n[Initializing Configuration]");
  logger.info("Loading configuration from JSON files");
  if (configManager.loadAll(config)) {
    configManager.printConfig(config);
    logger.success("Configuration loaded successfully");
  } else {
    logger.warning("Failed to load config, using defaults");
  }


  // Setup temperature sensors
  Serial.println("\n[Initializing Temperature Sensors]");
  logger.info("Initializing DS18B20 temperature sensors");
  sensors.begin();
  setupSensors();

  // Initialize web server object early to access WiFi methods
  webServer = new WebServerManager(&config, &sensorData, &pumpState);

  // Connect to WiFi and setup mDNS via WebServerManager
  webServer->connectWiFi();

  // Initialize real time if WiFi is available
  if (WiFi.status() == WL_CONNECTED) {
    if (initializeTime()) {
      logger.success("Real-time clock synchronized successfully");
    } else {
      logger.warning("Real-time clock synchronization failed, using uptime-based timestamps");
    }
  }

  // Setup web server
  Serial.println("\n[Initializing Web Server]");
  webServer->begin();

  // Add sensor list endpoint after web server starts
  // Now handled in WebServerManager::begin()

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║     SYSTEM READY                       ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.print("Web interface: http://");
    Serial.println(WiFi.localIP());
    Serial.printf("           or: http://%s.local\n", config.wifi.hostname);
    Serial.println("════════════════════════════════════════\n");
  }
}

void loop() {
  esp_task_wdt_reset(); // Pet the watchdog

  static unsigned long lastRead = 0;

  // Read temperatures every 2 seconds
  if (millis() - lastRead > 2000) {
    lastRead = millis();
    readTemperatures();

    // Update web server with latest sensor data
    if (webServer) {
      webServer->updateSensorData(sensorData.airTemp, sensorData.spaTemp, sensorData.panelTemp);
    }

    // Control pump based on temperatures (if not in manual mode)
    if (!config.temp.manualOverride) {
      controlPump();
    } else {
      // In manual mode, apply the configured state
      Serial.printf("config.temp.pumpState %d, pumpState %d\n", config.temp.pumpState, pumpState);
      bool desiredState = config.temp.pumpState;
      if (pumpState != desiredState) {
        pumpState = desiredState;
        digitalWrite(RELAY_PIN, pumpState ? HIGH : LOW);
      }
    }

    // Update web server with pump state
    if (webServer) {
      webServer->updatePumpState(pumpState);
    }

    // Move history recording outside of specific condition if needed
    // The internal interval check in recordHistory() will handle the 1-minute timing
    if (webServer) webServer->recordHistory();

    // Print status to serial
    Serial.printf("Air: %.1f°C | Spa: %.1f°C | Panel: %.1f°C | Diff: %.1f°C | Pump: %s | Mode: %s\n",
                  sensorData.airTemp,
                  sensorData.spaTemp,
                  sensorData.panelTemp,
                  sensorData.panelTemp - sensorData.spaTemp,
                  pumpState ? "ON " : "OFF",
                  config.temp.manualOverride ? "MANUAL" : "AUTO");
  }
}

void setupSensors() {
  // Request temperatures to wake up sensors on the bus
  sensors.requestTemperatures();
  delay(100); // Give sensors time to respond

  sensorCount = sensors.getDeviceCount();
  logger.infof("Found %d DS18B20 sensors on bus", sensorCount);

  if (sensorCount >= 3) {
    // Check if we have a sensor role mapping configured
    if (config.sensors.useMapping) {
      logger.info("Using configured sensor role mapping");

      // Copy configured addresses
      memcpy(airSensor, config.sensors.airSensorAddress, 8);
      memcpy(spaSensor, config.sensors.spaSensorAddress, 8);
      memcpy(panelSensor, config.sensors.panelSensorAddress, 8);

      Serial.println("\nUsing configured sensor roles:");
      Serial.print("  Air sensor:   ");
      for (int i = 0; i < 8; i++) Serial.printf("%02X ", airSensor[i]);
      Serial.println();

      Serial.print("  Spa sensor:   ");
      for (int i = 0; i < 8; i++) Serial.printf("%02X ", spaSensor[i]);
      Serial.println();

      Serial.print("  Panel sensor: ");
      for (int i = 0; i < 8; i++) Serial.printf("%02X ", panelSensor[i]);
      Serial.println();

      logger.success("Sensor roles loaded from configuration");
    } else {
      // Use default bus order
      logger.info("Using default sensor order (0=air, 1=spa, 2=panel)");

      sensors.getAddress(airSensor, 0);
      sensors.getAddress(spaSensor, 1);
      sensors.getAddress(panelSensor, 2);

      Serial.println("\nSensor addresses detected:");
      Serial.print("  [0] Air sensor:   ");
      for (int i = 0; i < 8; i++) Serial.printf("%02X ", airSensor[i]);
      Serial.println();

      Serial.print("  [1] Spa sensor:   ");
      for (int i = 0; i < 8; i++) Serial.printf("%02X ", spaSensor[i]);
      Serial.println();

      Serial.print("  [2] Panel sensor: ");
      for (int i = 0; i < 8; i++) Serial.printf("%02X ", panelSensor[i]);
      Serial.println();

      logger.success("All 3 sensors initialized with default order");
    }
  } else {
    logger.errorf("Only %d sensors detected (need 3)!", sensorCount);
    logger.warning("Check wiring and 4.7kΩ pull-up resistor");
  }
}

void readTemperatures() {
  sensors.requestTemperatures();

  if (sensorCount >= 3) {
    // Read raw temperatures and apply calibration offsets
    sensorData.airTemp = sensors.getTempC(airSensor) + config.sensors.airOffset;
    sensorData.spaTemp = sensors.getTempC(spaSensor) + config.sensors.spaOffset;
    sensorData.panelTemp = sensors.getTempC(panelSensor) + config.sensors.panelOffset;

    // Check for sensor errors
    if (sensorData.airTemp == DEVICE_DISCONNECTED_C) {
      sensorData.airTemp = 0.0;
      logger.warning("Air sensor disconnected!");
    }
    if (sensorData.spaTemp == DEVICE_DISCONNECTED_C) {
      sensorData.spaTemp = 0.0;
      logger.warning("Spa sensor disconnected!");
    }
    if (sensorData.panelTemp == DEVICE_DISCONNECTED_C) {
      sensorData.panelTemp = 0.0;
      logger.warning("Panel sensor disconnected!");
    }
  } else if (sensorCount > 0) {
    // Read whatever sensors are available for testing
    DeviceAddress tempAddr;
    for (int i = 0; i < sensorCount; i++) {
      sensors.getAddress(tempAddr, i);
      float temp = sensors.getTempC(tempAddr);
      if (i == 0) sensorData.airTemp = (temp != DEVICE_DISCONNECTED_C) ? temp : 0.0;
      if (i == 1) sensorData.spaTemp = (temp != DEVICE_DISCONNECTED_C) ? temp : 0.0;
      if (i == 2) sensorData.panelTemp = (temp != DEVICE_DISCONNECTED_C) ? temp : 0.0;
    }
  }
}

void controlPump() {
  bool shouldActivate = false;

  // Periodic sampling state
  static unsigned long lastSampleTime = 0;
  static bool isSampling = false;
  static unsigned long samplingStartedAt = 0;
  unsigned long now = millis();

  // Pump activation logic:
  // 1. External air temperature must be above minimum
  // 2. Spa temperature must be below maximum
  // 3. Panel must be warmer than spa by the threshold amount

  float tempDiff = sensorData.panelTemp - sensorData.spaTemp;

  if (sensorData.airTemp >= config.temp.minExternalTemp &&
      sensorData.spaTemp < config.temp.maxSpaTemp &&
      tempDiff >= config.temp.tempDifferenceThreshold) {
    shouldActivate = true;
  }

  // Apply hysteresis: once pump is on, require temperature difference
  // to drop below (threshold - hysteresis) before turning off.
  // External air must still be above minExternalTemp.
  if (pumpState &&
      sensorData.airTemp >= config.temp.minExternalTemp &&
      tempDiff >= (config.temp.tempDifferenceThreshold - config.temp.hysteresis)) {
    shouldActivate = true;
  }

  // 3. Periodic Sampling (Flush cycle)
  // Si la pompe est arrêtée et que la température d'air est favorable
  if (!shouldActivate && sensorData.airTemp >= config.temp.minExternalTemp && config.temp.sampleInterval > 0) {
    unsigned long intervalMs = (unsigned long)config.temp.sampleInterval * 60 * 1000;
    unsigned long durationMs = (unsigned long)config.temp.sampleDuration * 1000;

    if (!isSampling) {
      // Déclenchement si l'intervalle est écoulé ou au premier démarrage
      if (lastSampleTime == 0 || (now - lastSampleTime >= intervalMs)) {
        isSampling = true;
        samplingStartedAt = now;
        logger.info("Démarrage du cycle de purge pour mesure réelle");
      }
    }

    if (isSampling) {
      if (now - samplingStartedAt < durationMs) {
        shouldActivate = true; // Force la pompe ON
      } else {
        isSampling = false;
        lastSampleTime = now;
        logger.info("Cycle de purge terminé");
      }
    }
  } else if (shouldActivate) {
    // Si le chauffage démarre normalement, on réinitialise le minuteur de purge
    isSampling = false;
    lastSampleTime = now;
  }

  // Safety check: force pump off if spa is at or above max temp
  if (sensorData.spaTemp >= config.temp.maxSpaTemp) {
    shouldActivate = false;
    if (pumpState) {
      logger.errorf("SAFETY: Spa temp %.1f°C reached maximum (%.1f°C) - pump forced OFF",
                    sensorData.spaTemp, config.temp.maxSpaTemp);
    }
  }

  // Update pump state if changed
  if (shouldActivate != pumpState) {
    pumpState = shouldActivate;
    digitalWrite(RELAY_PIN, pumpState ? HIGH : LOW);

    if (pumpState) {
      logger.successf("Pump ACTIVATED (Panel %.1f°C > Spa %.1f°C + %.1f°C threshold)",
                      sensorData.panelTemp, sensorData.spaTemp, config.temp.tempDifferenceThreshold);
    } else {
      logger.infof("Pump DEACTIVATED (Temp diff: %.1f°C)", tempDiff);
    }
  }
}
void setRelay(bool on) {
  pumpState = on;
  Serial.printf("Setting relay: %s\n", on ? "ON" : "OFF");
  digitalWrite(RELAY_PIN, pumpState ? HIGH : LOW);
}

void printWelcome() {
  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════════════════════╗");
  Serial.println("║                                                        ║");
  Serial.println("║         CHAUFFAGE SOLAIRE SPA - ESP32                 ║");
  Serial.println("║         Solar Spa Heating Controller                  ║");
  Serial.println("║                                                        ║");
  Serial.printf("║         Version: %-35s║\n", FIRMWARE_VERSION);
  Serial.printf("║         Build: %s %s              ║\n", BUILD_DATE, BUILD_TIME);
  Serial.println("║                                                        ║");
  Serial.println("╚════════════════════════════════════════════════════════╝");
  Serial.println();
}
