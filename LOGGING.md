# System Logging - Guide

## Overview

The system includes a **centralized logging system** that captures all important events and displays them:
- ✅ In the **serial monitor** (real-time)
- ✅ On the **web interface** (last 100 entries)

This provides visibility into system operations, configuration changes, errors, and pump control events.

---

## Features

### Log Levels

The system supports 4 log levels:

| Level | Icon | Color | Usage |
|-------|------|-------|-------|
| **INFO** | ℹ️ | Blue | General information, status updates |
| **WARN** | ⚠️ | Yellow | Warnings, non-critical issues |
| **ERROR** | ❌ | Red | Errors, critical failures |
| **OK** | ✅ | Green | Successful operations |

### Log Storage

- **Capacity**: Last 100 log entries in memory
- **Circular buffer**: Oldest entries removed when full
- **Persistence**: Logs cleared on reboot (RAM-only)

### Display Locations

1. **Serial Monitor**: All logs with timestamps
2. **Web Interface**: Last 100 logs with auto-refresh

---

## Web Interface

### Logs Section

Located at the **bottom of the web page**, the logs section displays:

- **Timestamp**: HH:MM:SS format
- **Log level**: INFO, WARN, ERROR, OK
- **Icon**: Visual indicator
- **Message**: Detailed information

### Features

**Auto-refresh**: Updates every 3 seconds automatically

**Auto-scroll**: 
- Click "📌 Auto-scroll" button to toggle
- When enabled: automatically scrolls to newest logs
- When disabled (📍): stays at current position

**Clear logs**:
- Click "🗑️ Effacer" to clear all logs
- Requires confirmation
- Cannot be undone

**Manual refresh**:
- Click "🔄 Actualiser" to update immediately
- Useful when auto-refresh is disabled

### Visual Design

**Color-coded entries**:
- Blue background: INFO
- Yellow background: WARN
- Red background: ERROR
- Green background: OK

**Scrollable**: 400px height with vertical scroll

**Animation**: New entries slide in smoothly

---

## Serial Monitor

### Format

```
[HH:MM:SS] LEVEL Message
```

**Example**:
```
[00:05:23] INFO Loading configuration from JSON files
[00:05:24] OK Temperature configuration loaded from JSON
[00:05:24] OK WiFi configuration loaded from JSON
[00:05:25] OK All 3 sensors initialized successfully
[00:05:27] OK WiFi connected - IP: 192.168.1.100
[00:05:27] OK mDNS started: http://chauffeSpa.local
[00:05:28] OK HTTP server started on port 80
[00:05:30] OK Pump ACTIVATED (Panel 35.0°C > Spa 28.0°C + 5.0°C threshold)
```

---

## What Gets Logged

### System Startup

```
✅ Configuration loaded successfully
✅ All 3 sensors initialized successfully
✅ WiFi connected - IP: 192.168.1.100
✅ mDNS started: http://chauffeSpa.local
✅ LittleFS mounted successfully
✅ HTTP server started on port 80
```

### Configuration Changes

```
ℹ️ Temperature parameters updated: diff=7.0, min=30.0, max=38.0
✅ Temperature parameters saved
✅ WiFi configuration saved to JSON
⚠️ WiFi credentials updated - restart required
```

### Pump Control

```
ℹ️ Pump: MANUAL ON
ℹ️ Pump: MANUAL OFF
ℹ️ Pump: AUTO MODE
✅ Pump ACTIVATED (Panel 35.0°C > Spa 28.0°C + 5.0°C threshold)
ℹ️ Pump DEACTIVATED (Temp diff: 3.5°C)
```

### Sensor Errors

```
⚠️ Air sensor disconnected!
⚠️ Spa sensor disconnected!
⚠️ Panel sensor disconnected!
```

### Safety Events

```
❌ SAFETY: Spa temp 40.5°C reached maximum (40.0°C) - pump forced OFF
```

### System Errors

```
❌ LittleFS mount failed!
⚠️ Make sure to upload filesystem with: pio run --target uploadfs
❌ WiFi connection failed!
⚠️ Only 2 sensors detected (need 3)!
❌ Failed to parse temperature config: InvalidInput
```

---

## API Endpoints

### GET /logs

Returns logs as JSON array (last 100 entries).

**Response format**:
```json
[
  {
    "time": "00:05:23",
    "level": "INFO",
    "icon": "ℹ️",
    "message": "Loading configuration from JSON files"
  },
  {
    "time": "00:05:24",
    "level": "OK",
    "icon": "✅",
    "message": "Configuration loaded successfully"
  }
]
```

### POST /logs/clear

Clears all logs from memory.

**Response**: `200 OK` with "Logs cleared" message

---

## Programming Interface

### Using the Logger

Include the logger:
```cpp
#include "logger.h"
```

### Log Methods

**Info**:
```cpp
logger.info("System started");
logger.infof("Temperature: %.1f°C", temp);
```

**Warning**:
```cpp
logger.warning("Sensor disconnected");
logger.warningf("Low signal: %d dBm", rssi);
```

**Error**:
```cpp
logger.error("Failed to open file");
logger.errorf("Parse error: %s", error.c_str());
```

**Success**:
```cpp
logger.success("Configuration saved");
logger.successf("Connected to %s", ssid);
```

### Example Usage

```cpp
// Simple message
logger.info("Starting initialization");

// Formatted message
logger.infof("Found %d sensors", sensorCount);

// Error with details
logger.errorf("Failed to connect: %s", WiFi.status());

// Success notification
logger.success("System ready");
```

---

## Log Management

### Clear Logs

**Via Web Interface**:
1. Scroll to logs section
2. Click "🗑️ Effacer"
3. Confirm action

**Via API**:
```bash
curl -X POST http://chauffeSpa.local/logs/clear
```

**Programmatically**:
```cpp
logger.clear();  // Clears all logs
```

### Get Log Count

```cpp
int count = logger.getCount();
Serial.printf("Current logs: %d\n", count);
```

---

## Troubleshooting

### Logs Not Appearing in Web Interface

**Check**:
1. Web interface loaded? (open `http://chauffeSpa.local`)
2. JavaScript errors? (open browser console F12)
3. API endpoint working? (visit `http://chauffeSpa.local/logs`)

**Test**:
```bash
# Should return JSON array
curl http://chauffeSpa.local/logs
```

### Logs Show "Chargement des journaux..."

**Cause**: Fetch request failed

**Check**:
- Network connection
- ESP32 is running
- Web server started correctly

### Auto-scroll Not Working

**Solution**: Click the "📌 Auto-scroll" button to enable/disable

Icon meanings:
- 📌 = Auto-scroll **enabled** (scrolls to bottom)
- 📍 = Auto-scroll **disabled** (stays in place)

### Old Logs Disappearing

**Normal behavior**: Only last 100 logs kept in memory

**Why**: Limited RAM on ESP32

**Solution**: If you need history, save serial monitor output:
```bash
pio device monitor > logs.txt
```

---

## Performance Considerations

### Memory Usage

| Component | Size |
|-----------|------|
| Each log entry | ~100-200 bytes |
| 100 log entries | ~10-20 KB |
| Logger overhead | ~1 KB |
| **Total** | **~15-25 KB RAM** |

### Processing

- Logging adds **< 1ms** per message
- Web refresh adds **< 100ms** every 3 seconds
- Minimal impact on real-time operations

---

## Configuration

### Change Buffer Size

Edit `include/logger.h`:
```cpp
static const int MAX_LOGS = 100;  // Increase/decrease as needed
```

**Trade-offs**:
- More logs = more memory usage
- Fewer logs = less history

### Change Refresh Interval

Edit `data/index.html`:
```javascript
setInterval(updateLogs, 3000);  // Change 3000 to desired milliseconds
```

---

## Best Practices

### ✅ Do

- Use appropriate log levels
- Keep messages concise but informative
- Include relevant data (temperatures, states)
- Log important state changes
- Log errors with context

### ❌ Don't

- Log in tight loops (floods the buffer)
- Log sensitive information (passwords)
- Use Serial.println directly (use logger instead)
- Log excessively (performance impact)

---

## Examples

### Startup Sequence

```cpp
void setup() {
  Serial.begin(115200);

  logger.info("System starting");

  if (initSensors()) {
    logger.successf("Found %d sensors", sensorCount);
  } else {
    logger.error("Sensor initialization failed");
  }

  if (connectWiFi()) {
    logger.successf("WiFi connected - IP: %s",
                    WiFi.localIP().toString().c_str());
  } else {
    logger.error("WiFi connection failed");
  }

  logger.success("System ready");
}
```

### Error Handling

```cpp
void readConfig() {
  File file = LittleFS.open("/config.json", "r");

  if (!file) {
    logger.error("Failed to open config file");
    logger.warning("Using default configuration");
    return;
  }

  if (parseConfig(file)) {
    logger.success("Configuration loaded");
  } else {
    logger.error("Config parse error");
  }

  file.close();
}
```

### Pump Control

```cpp
void controlPump(bool activate) {
  if (activate) {
    digitalWrite(RELAY_PIN, HIGH);
    logger.successf("Pump ON (diff: %.1f°C)", tempDiff);
  } else {
    digitalWrite(RELAY_PIN, LOW);
    logger.info("Pump OFF");
  }
}
```

---

## Log Categories

### Informational (ℹ️)

- System state changes
- Normal operations
- Status updates
- Non-critical events

### Warnings (⚠️)

- Sensor disconnections
- Configuration issues
- Non-fatal errors
- Deprecated features

### Errors (❌)

- File system failures
- Network errors
- Parse failures
- Critical safety events

### Success (✅)

- Successful operations
- Completions
- Achievements
- Confirmations

---

## Future Enhancements

Potential improvements:

- [ ] Log persistence (save to file)
- [ ] Log levels filtering in web UI
- [ ] Download logs as file
- [ ] Email/push notifications for errors
- [ ] Log rotation (archive old logs)
- [ ] Search/filter functionality
- [ ] Log analytics/statistics

---

## Summary

| Feature | Status |
|---------|--------|
| **Serial logging** | ✅ All messages |
| **Web interface** | ✅ Last 100 entries |
| **Auto-refresh** | ✅ Every 3 seconds |
| **Color-coded** | ✅ By log level |
| **Timestamps** | ✅ HH:MM:SS |
| **Clear logs** | ✅ Via button/API |
| **Auto-scroll** | ✅ Toggle-able |
| **API access** | ✅ GET /logs |

---

**Version**: 2.2  
**Last updated**: 2026-04-13  
**Logger capacity**: 100 entries  
**Memory usage**: ~15-25 KB RAM
