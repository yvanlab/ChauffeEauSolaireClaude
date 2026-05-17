#ifndef WEBSERVER_H
#define WEBSERVER_H

// Define HTTP method constants before including ESPAsyncWebServer
// Workaround for library compatibility issues
#ifndef HTTP_GET
  #define HTTP_GET     0b00000001
  #define HTTP_POST    0b00000010
  #define HTTP_DELETE  0b00000100
  #define HTTP_PUT     0b00001000
  #define HTTP_PATCH   0b00010000
  #define HTTP_HEAD    0b00100000
  #define HTTP_OPTIONS 0b01000000
  #define HTTP_ANY     0b01111111
#endif

#include <ESPAsyncWebServer.h>
#include <stdint.h>
#include "config.h"

// Forward declaration for sensor data structure
struct SensorData {
  float airTemp;
  float spaTemp;
  float panelTemp;

  SensorData() : airTemp(0.0), spaTemp(0.0), panelTemp(0.0) {}
};

// Historical data point
struct TempDataPoint {
  uint64_t timestamp;  // epoch milliseconds
  float airTemp;
  float spaTemp;
  float panelTemp;
  bool pumpState;
};

// Tiered History Configuration
// Recent: 1 minute samples for the last 2 hours = 120 points
#define RECENT_HISTORY_POINTS 120
#define RECENT_HISTORY_INTERVAL_MS 60000 

// Archive: 30 minute samples for the rest of the day (approx 24h) = 48 points
#define ARCHIVE_HISTORY_POINTS 48
#define ARCHIVE_HISTORY_INTERVAL_MS 1800000 

class WebServerManager {
private:
  AsyncWebServer* server;
  SpaConfig* config;
  SensorData* sensorData;
  bool* pumpState;

  // Dual-buffer system for tiered sampling
  TempDataPoint recentHistoryBuffer[RECENT_HISTORY_POINTS];
  int recentHistoryCount;
  int recentHistoryIndex;
  unsigned long lastRecentHistoryUpdate;

  TempDataPoint archiveHistoryBuffer[ARCHIVE_HISTORY_POINTS];
  int archiveHistoryCount;
  int archiveHistoryIndex;
  unsigned long lastArchiveHistoryUpdate;

  // WiFi monitoring
  unsigned long lastWiFiCheck;

  // Route handlers
  void handleRoot(AsyncWebServerRequest *request);
  void handleData(AsyncWebServerRequest *request);
  void handleConfig(AsyncWebServerRequest *request);
  void handlePump(AsyncWebServerRequest *request);
  void handleWiFi(AsyncWebServerRequest *request);
  void handleReset(AsyncWebServerRequest *request);
  void handleHistory(AsyncWebServerRequest *request);
  void handleSensorMapping(AsyncWebServerRequest *request);
  void handleSensors(AsyncWebServerRequest *request);
  void handleDailyHistory(AsyncWebServerRequest *request);
  void handleDownloadDaily(AsyncWebServerRequest *request);

public:
  WebServerManager(SpaConfig* cfg, SensorData* data, bool* pump);
  ~WebServerManager();

  // Initialize and start the web server
  void begin();

  // Connect to WiFi and setup mDNS
  void connectWiFi();

  // Check WiFi connection status and reconnect if needed
  void checkWiFiConnection();

  // Stop the web server
  void end();

  // Update sensor data (called from main loop)
  void updateSensorData(float air, float spa, float panel);

  // Update pump state (called when pump changes)
  void updatePumpState(bool state);

  // Record current temperatures to history (called periodically)
  void recordHistory();

  // Record daily statistics to filesystem
  bool saveDailyStats(float minT, float maxT, float hours);

  // Get the server instance for adding custom routes
  AsyncWebServer* getServer() { return server; }
};

#endif // WEBSERVER_H
