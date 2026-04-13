#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <ESPAsyncWebServer.h>
#include "config.h"

// Forward declaration for sensor data structure
struct SensorData {
  float airTemp;
  float spaTemp;
  float panelTemp;

  SensorData() : airTemp(0.0), spaTemp(0.0), panelTemp(0.0) {}
};

class WebServerManager {
private:
  AsyncWebServer* server;
  SpaConfig* config;
  SensorData* sensorData;
  bool* pumpState;

  // Route handlers
  void handleRoot(AsyncWebServerRequest *request);
  void handleData(AsyncWebServerRequest *request);
  void handleConfig(AsyncWebServerRequest *request);
  void handlePump(AsyncWebServerRequest *request);
  void handleWiFi(AsyncWebServerRequest *request);
  void handleReset(AsyncWebServerRequest *request);

public:
  WebServerManager(SpaConfig* cfg, SensorData* data, bool* pump);
  ~WebServerManager();

  // Initialize and start the web server
  void begin();

  // Stop the web server
  void end();

  // Update sensor data (called from main loop)
  void updateSensorData(float air, float spa, float panel);

  // Update pump state (called when pump changes)
  void updatePumpState(bool state);
};

#endif // WEBSERVER_H
