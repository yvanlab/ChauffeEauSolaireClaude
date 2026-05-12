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

// Historical data buffer (24 hours, 1 point per minute = 1440 points max)
#define MAX_HISTORY_POINTS 1440
#define HISTORY_INTERVAL_MS 60000  // 1 minute

class WebServerManager {
private:
  AsyncWebServer* server;
  SpaConfig* config;
  SensorData* sensorData;
  bool* pumpState;

  // Historical data storage
  TempDataPoint* historyBuffer;
  int historyCount;
  int historyIndex;
  unsigned long lastHistoryUpdate;

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

public:
  WebServerManager(SpaConfig* cfg, SensorData* data, bool* pump);
  ~WebServerManager();

  // Initialize and start the web server
  void begin();

  // Connect to WiFi and setup mDNS
  void connectWiFi();

  // Stop the web server
  void end();

  // Update sensor data (called from main loop)
  void updateSensorData(float air, float spa, float panel);

  // Update pump state (called when pump changes)
  void updatePumpState(bool state);

  // Record current temperatures to history (called periodically)
  void recordHistory();

  // Get the server instance for adding custom routes
  AsyncWebServer* getServer() { return server; }
};

#endif // WEBSERVER_H
