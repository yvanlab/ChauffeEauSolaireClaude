#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LittleFS.h>
#include "config.h"
#include "webserver.h"
#include "logger.h"

// Version information
const char* FIRMWARE_VERSION = "2.4";
const char* BUILD_DATE = __DATE__;
const char* BUILD_TIME = __TIME__;

// Pin definitions
#define ONE_WIRE_BUS 4    // GPIO4 for DS18B20 sensors
#define RELAY_PIN 14      // GPIO14 for pump relay

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
void connectWiFi();
void printWelcome();

void setup() {
  Serial.begin(115200);
  delay(1000);

  printWelcome();

  // Setup relay pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // Pump off initially

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

  // Restore pump state if in manual mode
  if (config.temp.manualOverride) {
    pumpState = config.temp.pumpState;
    digitalWrite(RELAY_PIN, pumpState ? HIGH : LOW);
    logger.infof("Restored manual pump state: %s", pumpState ? "ON" : "OFF");
  }

  // Setup temperature sensors
  Serial.println("\n[Initializing Temperature Sensors]");
  logger.info("Initializing DS18B20 temperature sensors");
  sensors.begin();
  setupSensors();

  // Connect to WiFi and setup mDNS
  connectWiFi();

  // Setup web server
  Serial.println("\n[Initializing Web Server]");
  webServer = new WebServerManager(&config, &sensorData, &pumpState);
  webServer->begin();

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
      bool desiredState = config.temp.pumpState;
      if (pumpState != desiredState) {
        pumpState = desiredState;
        digitalWrite(RELAY_PIN, pumpState ? HIGH : LOW);
      }
    }

    // Update web server with pump state
    if (webServer) {
      webServer->updatePumpState(pumpState);
      // Record temperature history (every minute)
      webServer->recordHistory();
    }

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
  sensorCount = sensors.getDeviceCount();
  logger.infof("Found %d DS18B20 sensors on bus", sensorCount);

  if (sensorCount >= 3) {
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

    logger.success("All 3 sensors initialized successfully");
  } else {
    logger.errorf("Only %d sensors detected (need 3)!", sensorCount);
    logger.warning("Check wiring and 4.7kΩ pull-up resistor");
  }
}

void readTemperatures() {
  sensors.requestTemperatures();

  if (sensorCount >= 3) {
    sensorData.airTemp = sensors.getTempC(airSensor);
    sensorData.spaTemp = sensors.getTempC(spaSensor);
    sensorData.panelTemp = sensors.getTempC(panelSensor);

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

  // Pump activation logic:
  // 1. Panel temperature must be above minimum
  // 2. Spa temperature must be below maximum
  // 3. Panel must be warmer than spa by the threshold amount

  float tempDiff = sensorData.panelTemp - sensorData.spaTemp;

  if (sensorData.panelTemp >= config.temp.minPanelTemp &&
      sensorData.spaTemp < config.temp.maxSpaTemp &&
      tempDiff >= config.temp.tempDifferenceThreshold) {
    shouldActivate = true;
  }

  // Apply hysteresis: once pump is on, require temperature difference
  // to drop below (threshold - 1) before turning off.
  // Panel must still be above minPanelTemp to avoid circulating cold water.
  if (pumpState &&
      sensorData.panelTemp >= config.temp.minPanelTemp &&
      tempDiff >= (config.temp.tempDifferenceThreshold - 1.0)) {
    shouldActivate = true;
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

void connectWiFi() {
  Serial.println("\n[Connecting to WiFi]");
  Serial.printf("SSID: %s\n", config.wifi.ssid);
  Serial.printf("Hostname: %s.local\n", config.wifi.hostname);

  logger.infof("Connecting to WiFi: %s", config.wifi.ssid);

  // Set hostname before connecting
  WiFi.setHostname(config.wifi.hostname);
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifi.ssid, config.wifi.password);

  int attempts = 0;
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    logger.successf("WiFi connected - IP: %s", WiFi.localIP().toString().c_str());
    logger.infof("Signal strength: %d dBm", WiFi.RSSI());

    Serial.print("  IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("  Signal strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    // Start mDNS responder
    if (MDNS.begin(config.wifi.hostname)) {
      logger.successf("mDNS started: http://%s.local", config.wifi.hostname);

      // Add service to mDNS-SD
      MDNS.addService("http", "tcp", 80);
      Serial.println("  Service added: _http._tcp");
    } else {
      logger.error("Failed to start mDNS responder");
      logger.info("You can still access via IP address");
    }
  } else {
    logger.error("WiFi connection failed!");
    logger.warning("System will continue without WiFi");
    logger.warning("Web interface will not be available");
  }
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
