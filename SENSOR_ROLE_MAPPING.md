# Sensor Role Mapping Feature

## Overview
Added the ability to associate DS18B20 temperature sensors with their roles (Air, Spa, Panel) through the web UI. The configuration is stored in a JSON file and persists across reboots.

## What Was Added

### 1. Configuration Structure (`include/config.h`)
- Added `SensorMapping` struct to store sensor addresses and mapping state
- Integrated into `SpaConfig` structure
- Added methods to `ConfigManager` for loading/saving sensor mappings

### 2. Backend Implementation

#### config.cpp
- New file: `/sensor_config.json` stores sensor role mappings
- `loadSensorMapping()`: Loads sensor-to-role associations from JSON
- `saveSensorMapping()`: Saves sensor-to-role associations to JSON
- Updated `loadAll()` to include sensor configuration

#### webserver.cpp
- Added `/sensors` endpoint: Returns list of all detected sensors with current roles and temperatures
- Added `/sensors/mapping` endpoint: Saves sensor role assignments
- Added `/restart` endpoint: Restarts the ESP32
- Helper function `parseAddress()`: Parses hex address strings (e.g., "28-FF-AA-BB-CC-DD-EE-01")

#### main.cpp
- Updated `setupSensors()` to use configured sensor mapping when available
- Falls back to default bus order (0=air, 1=spa, 2=panel) if no mapping configured
- Registers `/sensors` endpoint handler after web server initialization
- Endpoint provides real-time sensor data including current role assignments

### 3. Frontend UI (`data/index.html`)

#### New Tab: "Capteurs"
- Displays all detected DS18B20 sensors
- Shows for each sensor:
  - Unique address (64-bit hex ID)
  - Current temperature reading
  - Current role assignment
  - Dropdown to change role

#### Features:
- Color-coded role badges:
  - 🌡️ Air (red)
  - 💧 Spa (blue)
  - ☀️ Panneaux (yellow)
  - Unassigned (gray)
- Real-time temperature display for each sensor
- Validation: Ensures all three roles are assigned before saving
- Prevents duplicate role assignments
- Auto-refresh capability
- Prompts for restart after saving configuration

#### CSS Additions:
- `.sensor-item`: Card-style display for each sensor
- `.sensor-address`: Monospace font for addresses
- `.role-badge`: Color-coded role indicators
- Hover effects and transitions

#### JavaScript Functions:
- `loadSensors()`: Fetches sensor list from `/sensors` endpoint
- `displaySensors()`: Renders sensor list with role dropdowns
- `updateSensorRole()`: Updates badge when role changes
- `saveSensorMapping()`: Validates and saves configuration
- `getRoleLabel()`: Returns localized role names

## Configuration File Format

### `/sensor_config.json`
```json
{
  "useMapping": true,
  "airSensor": [40, 255, 170, 187, 204, 221, 238, 1],
  "spaSensor": [40, 255, 170, 187, 204, 221, 238, 2],
  "panelSensor": [40, 255, 170, 187, 204, 221, 238, 3]
}
```

- `useMapping`: Boolean flag to enable custom mapping
- `airSensor`, `spaSensor`, `panelSensor`: 8-byte arrays representing DS18B20 addresses

## Usage Flow

1. **Initial Setup** (no mapping configured):
   - System uses default bus order: 0=air, 1=spa, 2=panel
   - `useMapping` is false

2. **Configure Sensor Roles**:
   - Navigate to "Capteurs" tab in web interface
   - View all detected sensors with current temperatures
   - Assign each sensor to a role using dropdown menus
   - Click "Enregistrer la configuration"
   - System validates all three roles are assigned
   - Configuration saved to `/sensor_config.json`
   - `useMapping` set to true

3. **After Restart**:
   - System loads sensor mapping from JSON
   - Sensors read according to configured roles
   - Correct temperature displayed for each role

4. **Modify Mapping**:
   - Return to "Capteurs" tab
   - Change role assignments as needed
   - Save and restart

## Benefits

1. **Flexibility**: Sensors can be physically installed in any order
2. **Reliability**: Sensor addresses are unique and don't change
3. **User-Friendly**: No need to recompile code or edit config files manually
4. **Validation**: Prevents invalid configurations (missing roles, duplicates)
5. **Persistence**: Configuration survives reboots and firmware updates (if filesystem preserved)
6. **Visibility**: Shows current temperature of each sensor to aid identification

## Technical Details

### Sensor Address Format
- DS18B20 sensors have 64-bit unique ROM addresses
- Format: `[Family Code (1 byte)][Serial Number (6 bytes)][CRC (1 byte)]`
- Displayed as hex: `28-FF-AA-BB-CC-DD-EE-01`
- Stored as 8-byte array in JSON

### Integration Points
- `config.sensors`: SensorMapping structure in main config
- `configManager.loadSensorMapping()`: Load on startup
- `configManager.saveSensorMapping()`: Save from web UI
- `setupSensors()`: Apply mapping during initialization
- `/sensors`: List available sensors
- `/sensors/mapping`: Save role assignments

### Error Handling
- Falls back to default order if JSON parsing fails
- Validates address format before saving
- Checks all roles assigned before accepting configuration
- Logs sensor mapping status at startup

## API Endpoints

### GET `/sensors`
Returns list of detected sensors with current roles and temperatures.

**Response:**
```json
{
  "sensors": [
    {
      "index": 0,
      "address": "28-FF-AA-BB-CC-DD-EE-01",
      "role": "air",
      "temp": 22.5
    },
    {
      "index": 1,
      "address": "28-FF-AA-BB-CC-DD-EE-02",
      "role": "spa",
      "temp": 35.2
    },
    {
      "index": 2,
      "address": "28-FF-AA-BB-CC-DD-EE-03",
      "role": "panel",
      "temp": 45.8
    }
  ],
  "useMapping": true,
  "count": 3
}
```

### POST `/sensors/mapping`
Saves sensor role assignments.

**Parameters:**
- `airAddr`: Hex address of air sensor (e.g., "28-FF-AA-BB-CC-DD-EE-01")
- `spaAddr`: Hex address of spa sensor
- `panelAddr`: Hex address of panel sensor

**Response:**
- `200 OK`: "Sensor mapping saved - restart required"
- `400 Bad Request`: "Invalid sensor mapping parameters"

## Files Modified

1. `include/config.h` - Added SensorMapping struct and methods
2. `src/config.cpp` - Implemented load/save for sensor mapping
3. `include/webserver.h` - Added sensor endpoint handlers
4. `src/webserver.cpp` - Implemented sensor endpoints and restart
5. `src/main.cpp` - Updated sensor initialization to use mapping
6. `data/index.html` - Added sensor configuration tab and UI

## Compilation Status
✅ Successfully compiled (Flash: 69.5%, RAM: 14.1%)

## Next Steps
1. Upload firmware to ESP32: `pio run --target upload`
2. Upload filesystem: `pio run --target uploadfs`
3. Navigate to Capteurs tab to configure sensor roles
4. Restart ESP32 after saving configuration
