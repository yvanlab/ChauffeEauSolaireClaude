# WiFi Connection Loss Analysis

## Problem Statement
WiFi connection is lost after several hours of operation, requiring ESP32 restart to reconnect.

---

## Root Causes Identified

### 🔴 **CRITICAL: No WiFi Reconnection Logic in Main Loop**

**Location**: `src/main.cpp` - `loop()` function (lines 135-181)

**Problem**: 
- WiFi connection is established once during `setup()` via `webServer->connectWiFi()`
- **NO monitoring or reconnection logic in `loop()`**
- If WiFi drops during runtime, the system never attempts to reconnect
- Web interface becomes inaccessible until manual restart

**Evidence**:
```cpp
void loop() {
  esp_task_wdt_reset(); // Pet the watchdog
  
  static unsigned long lastRead = 0;
  
  // Read temperatures every 2 seconds
  if (millis() - lastRead > 2000) {
    // ... temperature reading and pump control ...
  }
  // ❌ NO WiFi status check
  // ❌ NO reconnection attempt
}
```

---

### 🔴 **CRITICAL: No WiFi Auto-Reconnect Enabled**

**Location**: `src/webserver.cpp` - `connectWiFi()` function (lines 287-340)

**Problem**:
- `WiFi.setAutoReconnect()` is **never called**
- ESP32 WiFi library defaults to **NO auto-reconnect**
- Even minor network disruptions cause permanent disconnection

**Current Code**:
```cpp
void WebServerManager::connectWiFi() {
  WiFi.setHostname(config->wifi.hostname);
  WiFi.mode(WIFI_STA);
  WiFi.begin(config->wifi.ssid, config->wifi.password);
  
  // ❌ Missing: WiFi.setAutoReconnect(true);
  // ❌ Missing: WiFi.persistent(true);
  
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    attempts++;
  }
}
```

---

### 🟡 **HIGH: No mDNS Update in Loop**

**Location**: `src/main.cpp` - `loop()` function

**Problem**:
- mDNS responder started in `connectWiFi()` but **never updated**
- mDNS requires periodic `MDNS.update()` or equivalent to maintain hostname resolution
- After WiFi reconnection, mDNS may not work without re-initialization

**Missing Code**:
```cpp
void loop() {
  // ❌ Missing: MDNS.update() or equivalent service
}
```

**Note**: Modern ESP32 Arduino Core (>= 2.0.0) handles this automatically, but older versions require manual updates.

---

### 🟡 **MEDIUM: Blocking WiFi Connection During Setup**

**Location**: `src/webserver.cpp:301-306`

**Problem**:
- Initial WiFi connection uses blocking `delay(500)` in a `while` loop
- Can delay startup by up to 15 seconds (30 attempts × 500ms)
- Watchdog timer is reset, but this blocks all other initialization

**Current Code**:
```cpp
while (WiFi.status() != WL_CONNECTED && attempts < 30) {
  esp_task_wdt_reset(); // Pet the watchdog
  delay(500);           // ⚠️ Blocking delay
  Serial.print(".");
  attempts++;
}
```

**Impact**: Not directly causing WiFi loss, but prevents proper async handling of WiFi events.

---

### 🟢 **LOW: No WiFi Event Handlers**

**Location**: Missing from entire codebase

**Problem**:
- ESP32 supports WiFi event callbacks for connection/disconnection events
- Current code has **no event handlers** registered
- Cannot proactively detect and respond to WiFi state changes

**Missing Implementation**:
```cpp
// ❌ Not implemented anywhere
WiFi.onEvent(WiFiEvent);  // Register event handler

void WiFiEvent(WiFiEvent_t event) {
  switch(event) {
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      logger.warning("WiFi disconnected - attempting reconnect");
      // Trigger reconnection logic
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      logger.success("WiFi reconnected");
      break;
  }
}
```

---

### 🟢 **LOW: Long-Running Process Without Network Activity**

**Location**: `src/main.cpp:135-181`

**Problem**:
- Main loop runs continuously with 2-second sensor reads
- No periodic network activity to keep connection alive
- Some routers may drop idle connections after several hours
- Web requests are client-initiated (no keep-alive from ESP32)

**Contributing Factors**:
- No periodic HTTP requests from ESP32
- No MQTT or other persistent connections
- Router may assume device is offline after extended idle period

---

## Failure Scenario Timeline

### Hour 0 - Startup
```
✅ WiFi.begin() connects successfully
✅ mDNS started
✅ Web server responding
```

### Hours 1-4 - Normal Operation
```
✅ Sensors reading every 2 seconds
✅ Web interface accessible
✅ Pump control working
⚠️  No WiFi status checks occurring
⚠️  No reconnection logic active
```

### Hour 5+ - Connection Lost
```
❌ Router reboots / WiFi interference / channel change
❌ ESP32 WiFi disconnected
❌ NO auto-reconnect enabled
❌ NO loop() monitoring detects disconnection
❌ Web interface inaccessible
❌ mDNS resolution fails
⚠️  Device continues operating (sensors + pump still work)
⚠️  Serial monitor still shows temperature readings
💀 WiFi connection permanently lost until manual restart
```

---

## Recommended Fixes

### ✅ **Fix 1: Enable Auto-Reconnect (CRITICAL)**

**File**: `src/webserver.cpp` - `connectWiFi()` function

```cpp
void WebServerManager::connectWiFi() {
  Serial.println("\n[Connecting to WiFi]");
  Serial.printf("SSID: %s\n", config->wifi.ssid);
  
  logger.infof("Connecting to WiFi: %s", config->wifi.ssid);
  
  // Set hostname before connecting
  WiFi.setHostname(config->wifi.hostname);
  WiFi.mode(WIFI_STA);
  
  // ✅ CRITICAL FIX: Enable auto-reconnect
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);  // Save WiFi config to flash
  
  WiFi.begin(config->wifi.ssid, config->wifi.password);
  
  // ... rest of connection logic ...
}
```

**Impact**: ESP32 will automatically attempt to reconnect when connection is lost.

---

### ✅ **Fix 2: Add WiFi Monitoring to Main Loop (CRITICAL)**

**File**: `src/main.cpp` - `loop()` function

```cpp
void loop() {
  esp_task_wdt_reset(); // Pet the watchdog
  
  static unsigned long lastRead = 0;
  static unsigned long lastWiFiCheck = 0;
  
  // ✅ NEW: Check WiFi status every 30 seconds
  if (millis() - lastWiFiCheck > 30000) {
    lastWiFiCheck = millis();
    
    if (WiFi.status() != WL_CONNECTED) {
      logger.warning("WiFi disconnected - attempting reconnect");
      webServer->connectWiFi();
    }
  }
  
  // Read temperatures every 2 seconds
  if (millis() - lastRead > 2000) {
    lastRead = millis();
    readTemperatures();
    
    // ... rest of loop code ...
  }
}
```

**Alternative (Better)**: Create a dedicated WiFi monitoring method:

```cpp
// Add to webserver.h
class WebServerManager {
  // ...
  void checkWiFiConnection();  // ✅ NEW
  unsigned long lastWiFiCheck = 0;
};

// Add to webserver.cpp
void WebServerManager::checkWiFiConnection() {
  unsigned long now = millis();
  
  // Check every 30 seconds
  if (now - lastWiFiCheck < 30000) {
    return;
  }
  
  lastWiFiCheck = now;
  
  if (WiFi.status() != WL_CONNECTED) {
    logger.warning("WiFi disconnected - attempting reconnect");
    WiFi.reconnect();  // Use reconnect() instead of full connectWiFi()
    
    // Wait for reconnection (non-blocking)
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      logger.success("WiFi reconnected successfully");
    } else {
      logger.error("WiFi reconnection failed");
    }
  }
}

// Call from main.cpp loop()
void loop() {
  // ...
  if (webServer) {
    webServer->checkWiFiConnection();
  }
  // ...
}
```

---

### ✅ **Fix 3: Register WiFi Event Handlers (RECOMMENDED)**

**File**: `src/webserver.cpp` - Add to beginning of file

```cpp
// ✅ NEW: WiFi event handler (static function)
static void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch(event) {
    case ARDUINO_EVENT_WIFI_STA_START:
      logger.info("WiFi station started");
      break;
      
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      logger.success("WiFi connected to AP");
      break;
      
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      logger.successf("WiFi connected - IP: %s", WiFi.localIP().toString().c_str());
      logger.infof("Signal strength: %d dBm", WiFi.RSSI());
      break;
      
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      logger.warning("WiFi disconnected - auto-reconnect will attempt to restore connection");
      break;
      
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      logger.error("WiFi lost IP address");
      break;
      
    default:
      break;
  }
}

// Add to connectWiFi() function
void WebServerManager::connectWiFi() {
  // Register event handler BEFORE connecting
  WiFi.onEvent(onWiFiEvent);
  
  // ... rest of connection code ...
}
```

**Benefits**:
- Proactive notification of WiFi state changes
- Better debugging and logging
- Can trigger custom recovery actions

---

### ✅ **Fix 4: Add mDNS Update (if needed)**

**File**: `src/main.cpp` - `loop()` function

```cpp
void loop() {
  esp_task_wdt_reset();
  
  // ✅ NEW: Update mDNS responder (only needed for older cores)
  #if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR < 2
    MDNS.update();  // Only needed for ESP32 Arduino Core < 2.0
  #endif
  
  // ... rest of loop code ...
}
```

**Note**: ESP32 Arduino Core >= 2.0.0 handles mDNS automatically in background task. Check your core version:
```cpp
Serial.printf("ESP32 Core Version: %d.%d.%d\n", 
              ESP_ARDUINO_VERSION_MAJOR, 
              ESP_ARDUINO_VERSION_MINOR, 
              ESP_ARDUINO_VERSION_PATCH);
```

---

### ✅ **Fix 5: Improve Initial Connection (Optional)**

**File**: `src/webserver.cpp` - `connectWiFi()`

Replace blocking delay with non-blocking alternative:

```cpp
void WebServerManager::connectWiFi() {
  WiFi.setHostname(config->wifi.hostname);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.begin(config->wifi.ssid, config->wifi.password);
  
  // ✅ Non-blocking connection with timeout
  unsigned long startAttempt = millis();
  Serial.print("Connecting");
  
  while (WiFi.status() != WL_CONNECTED && 
         millis() - startAttempt < 15000) {  // 15 second timeout
    esp_task_wdt_reset();
    delay(100);  // Shorter delay for better responsiveness
    Serial.print(".");
    yield();  // Allow background tasks to run
  }
  Serial.println();
  
  // ... rest of function ...
}
```

---

## Testing the Fixes

### Test 1: Verify Auto-Reconnect
```
1. Upload firmware with Fix 1
2. Monitor serial output
3. Reboot your router
4. Observe ESP32 automatically reconnects
5. Check web interface becomes accessible again
```

### Test 2: Long-Running Stability
```
1. Apply Fixes 1, 2, and 3
2. Let device run for 24+ hours
3. Monitor logs for disconnection events
4. Verify auto-reconnect occurs
5. Check web interface remains accessible
```

### Test 3: WiFi Interference
```
1. Apply all fixes
2. Move ESP32 far from router (weak signal)
3. Observe reconnection behavior
4. Check logs show disconnection and reconnection
```

---

## Additional Recommendations

### 1. **Add Connection Status to Web UI**
Display WiFi status prominently:
```javascript
// In index.html
function updateWiFiStatus() {
  fetch('/data')
    .then(response => response.json())
    .then(data => {
      document.getElementById('wifi-status').textContent = 
        data.wifiRSSI ? `Connected (${data.wifiRSSI} dBm)` : 'Disconnected';
    })
    .catch(() => {
      document.getElementById('wifi-status').textContent = 'Disconnected';
    });
}
```

### 2. **Log WiFi Disconnection Events**
Add to system logs when WiFi drops - already supported via `logger.warning()` in event handler.

### 3. **Watchdog for Web Server**
Add timeout detection:
```cpp
static unsigned long lastWebRequest = 0;

// In request handlers
lastWebRequest = millis();

// In loop()
if (millis() - lastWebRequest > 3600000) {  // 1 hour no requests
  logger.warning("No web requests in 1 hour - connection may be lost");
}
```

### 4. **Periodic Keep-Alive**
Send mDNS announcements periodically:
```cpp
// Every 5 minutes
if (millis() - lastMDNSAnnounce > 300000) {
  MDNS.announce();  // Re-announce presence
  lastMDNSAnnounce = millis();
}
```

---

## Priority Implementation Order

1. **✅ IMMEDIATE**: Fix 1 - Enable `WiFi.setAutoReconnect(true)`
2. **✅ IMMEDIATE**: Fix 2 - Add WiFi monitoring to loop
3. **✅ HIGH**: Fix 3 - Register WiFi event handlers
4. **🟡 MEDIUM**: Fix 4 - Add mDNS update (check core version first)
5. **🟢 LOW**: Fix 5 - Improve initial connection logic

---

## Files to Modify

1. `src/webserver.cpp` - Add auto-reconnect + event handlers
2. `src/webserver.h` - Add `checkWiFiConnection()` method
3. `src/main.cpp` - Call WiFi check in loop + mDNS update

---

## Expected Results After Fixes

### Before Fixes
```
⏰ Hour 0: WiFi connected
⏰ Hour 1-5: Normal operation
💀 Hour 6: Router reboots → WiFi lost forever
🚫 Web interface inaccessible until manual restart
```

### After Fixes
```
⏰ Hour 0: WiFi connected
⏰ Hour 1-5: Normal operation
⚠️  Hour 6: Router reboots → WiFi lost
✅ Hour 6+30s: Auto-reconnect triggered
✅ Hour 6+1m: WiFi restored, web interface accessible
📊 Logs show disconnect/reconnect event
```

---

## Summary

**Root Cause**: No WiFi reconnection logic - connection lost after several hours is permanent until restart.

**Solution**: Enable auto-reconnect + add monitoring loop + event handlers.

**Effort**: Low (3 small code changes)

**Impact**: High (resolves permanent WiFi loss issue)

---

**Generated**: 2026-05-13  
**Version**: Firmware 2.4 Analysis
