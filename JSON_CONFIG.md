# JSON Configuration Guide

## Overview

The system uses **JSON files** stored in LittleFS for configuration management instead of NVS (Non-Volatile Storage). This provides:

✅ **Human-readable** configuration files  
✅ **Easy backup/restore** - just copy JSON files  
✅ **Direct editing** possible (via filesystem upload)  
✅ **Version control friendly** - can track config changes in git  
✅ **Separate concerns** - temperature and WiFi configs in different files  

---

## Configuration Files

### Location

All configuration files are stored in the **data/** folder and uploaded to ESP32's LittleFS:

```
data/
├── index.html           # Web interface
├── temp_config.json     # Temperature & pump configuration
└── wifi_config.json     # WiFi & network configuration
```

---

## 1. Temperature Configuration

**File**: `data/temp_config.json`

```json
{
  "tempDifferenceThreshold": 5.0,
  "minPanelTemp": 25.0,
  "maxSpaTemp": 40.0,
  "manualOverride": false,
  "pumpState": false
}
```

### Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `tempDifferenceThreshold` | float | 5.0 | Temperature difference (°C) between panel and spa to activate pump |
| `minPanelTemp` | float | 25.0 | Minimum panel temperature (°C) required to activate pump |
| `maxSpaTemp` | float | 40.0 | Maximum spa temperature (°C) - safety limit |
| `manualOverride` | bool | false | Manual pump control mode active |
| `pumpState` | bool | false | Pump state when in manual mode |

### When It's Modified

- **Via web interface**: Configuration tab
- **Via web interface**: Manual pump control buttons
- **Automatically**: When pump state changes in manual mode

### Persistence

- ✅ Saved automatically when changed via web
- ✅ Restored on ESP32 restart
- ✅ Survives power loss

---

## 2. WiFi Configuration

**File**: `data/wifi_config.json`

```json
{
  "ssid": "YOUR_WIFI_SSID",
  "password": "YOUR_WIFI_PASSWORD",
  "hostname": "chauffeSpa"
}
```

### Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `ssid` | string | "YOUR_WIFI_SSID" | WiFi network name |
| `password` | string | "YOUR_WIFI_PASSWORD" | WiFi password |
| `hostname` | string | "chauffeSpa" | mDNS hostname (access via `hostname.local`) |

### When It's Modified

- **Via web interface**: WiFi configuration tab
- **Manually**: Edit file before upload

### Persistence

- ✅ Saved when changed via web (requires restart)
- ✅ Can be edited directly in `data/wifi_config.json`

---

## Initial Setup

### Method 1: Edit JSON Files Before Upload (Recommended)

1. **Edit WiFi config**:
   ```bash
   # Edit data/wifi_config.json
   {
     "ssid": "MyHomeNetwork",
     "password": "MySecurePassword",
     "hostname": "chauffeSpa"
   }
   ```

2. **Edit temperature config** (optional):
   ```bash
   # Edit data/temp_config.json
   {
     "tempDifferenceThreshold": 7.0,
     "minPanelTemp": 30.0,
     "maxSpaTemp": 38.0,
     "manualOverride": false,
     "pumpState": false
   }
   ```

3. **Upload to ESP32**:
   ```bash
   pio run --target upload       # Upload program
   pio run --target uploadfs     # Upload JSON configs
   ```

### Method 2: Use Defaults Then Configure Via Web

1. **Upload with defaults**:
   ```bash
   pio run --target upload
   pio run --target uploadfs
   ```

2. **Connect to default WiFi** (if you set it in the files)

3. **Access web interface** and configure

---

## Accessing the System

### By IP Address

```
http://192.168.1.xxx
```

Find IP in serial monitor output.

### By Hostname (mDNS)

```
http://chauffeSpa.local
```

**Requirements**:
- ✅ Works on: Windows 10+, macOS, Linux, iOS, Android
- ✅ Same network as ESP32
- ✅ mDNS service running (usually automatic)

**Advantages**:
- No need to remember IP address
- IP can change, hostname stays the same
- Easier bookmarking

### Custom Hostname

To use a different hostname, edit `wifi_config.json`:

```json
{
  "ssid": "MyNetwork",
  "password": "password",
  "hostname": "myCustomName"
}
```

Then access via: `http://myCustomName.local`

**Hostname rules**:
- Lowercase letters and numbers
- No spaces or special characters
- Max 32 characters
- No `.local` suffix (added automatically)

---

## Runtime Configuration

### Via Web Interface

All configuration can be modified through the web interface:

#### Temperature Parameters
1. Open `http://chauffeSpa.local`
2. Go to **Configuration** → **Températures** tab
3. Modify values
4. Click **Enregistrer**
5. ✅ Saved immediately to `temp_config.json`

#### WiFi Settings
1. Open `http://chauffeSpa.local`
2. Go to **Configuration** → **WiFi** tab
3. Enter new SSID/password
4. Click **Enregistrer WiFi**
5. ⚠️ **Restart required** to apply

#### Pump Control
1. Use **Contrôle manuel** buttons
2. State automatically saved to `temp_config.json`
3. ✅ Restored after reboot

---

## Backup & Restore

### Backup Configuration

#### Method 1: Copy from Repository

Before uploading, the `data/` folder contains your config:

```bash
# Backup locally
cp data/temp_config.json backup/
cp data/wifi_config.json backup/
```

#### Method 2: Download from ESP32

Unfortunately, downloading from ESP32 filesystem is not straightforward. Best practice: **keep your `data/` folder in version control**.

### Restore Configuration

1. Copy backup files to `data/` folder:
   ```bash
   cp backup/*.json data/
   ```

2. Upload to ESP32:
   ```bash
   pio run --target uploadfs
   ```

3. Restart ESP32

---

## Reset to Defaults

### Via Web Interface

1. Go to **Configuration** → **Système**
2. Click **Réinitialiser**
3. Confirms and resets both JSON files to defaults
4. Restart recommended

### Via Serial Monitor

If web interface is inaccessible, you'll need to:

1. Edit JSON files in `data/` folder
2. Re-upload filesystem:
   ```bash
   pio run --target uploadfs
   ```

---

## JSON File Format Details

### Valid JSON

Ensure proper JSON syntax:

```json
{
  "key": "value",
  "number": 5.0,
  "boolean": true,
  "lastKey": "noCommaAfter"
}
```

### Common Mistakes

❌ **Trailing comma**:
```json
{
  "maxSpaTemp": 40.0,  ← Remove this comma
}
```

❌ **Missing quotes**:
```json
{
  ssid: "network"  ← Should be "ssid"
}
```

❌ **Comments not allowed**:
```json
{
  // This comment will break parsing
  "ssid": "network"
}
```

✅ **Correct format**:
```json
{
  "ssid": "network",
  "password": "pass"
}
```

---

## Troubleshooting

### Configuration Not Loading

**Serial monitor shows**:
```
⚠ Temperature config file not found: /temp_config.json
  Using default values
```

**Solution**:
```bash
# Upload filesystem
pio run --target uploadfs
```

### JSON Parse Error

**Serial monitor shows**:
```
✗ Failed to parse temperature config: InvalidInput
```

**Possible causes**:
1. Invalid JSON syntax
2. Missing comma or quote
3. File corruption

**Solution**:
1. Validate JSON online: https://jsonlint.com/
2. Fix syntax errors
3. Re-upload:
   ```bash
   pio run --target uploadfs
   ```

### Changes Not Persisting

**Symptom**: Configuration resets after restart

**Possible causes**:
1. LittleFS not mounted
2. Write failed (filesystem full/corrupted)
3. Wrong file path

**Check serial monitor**:
```
✓ Temperature configuration saved to JSON  ← Should see this
```

**Solution**:
1. Verify LittleFS mounted: `✓ LittleFS mounted successfully`
2. Check free space (JSON files are tiny, should not be issue)
3. Re-upload filesystem if corrupted

### mDNS Not Working

**Cannot access `http://chauffeSpa.local`**

**Serial monitor shows**:
```
✗ Failed to start mDNS responder
```

**Solutions**:
1. **Use IP address instead** (always works)
2. Check if another device uses same hostname
3. Verify WiFi connected successfully
4. Restart ESP32
5. Try different mDNS-compatible browser

**On Windows**:
- Install Bonjour Print Services if mDNS doesn't work
- Or use IP address

---

## File Size & Limits

### Current Usage

| File | Size | Max Size |
|------|------|----------|
| `temp_config.json` | ~120 bytes | 64 KB |
| `wifi_config.json` | ~100 bytes | 64 KB |
| **Total** | **~220 bytes** | LittleFS has 1.5 MB |

### String Limits

Defined in code (`config.h`):

```cpp
char ssid[64];        // Max 63 characters + null
char password[64];    // Max 63 characters + null
char hostname[32];    // Max 31 characters + null
```

---

## Advanced: Direct File Editing

### Edit Configuration Without Web Interface

1. **Modify JSON in `data/` folder**:
   ```bash
   notepad data/temp_config.json
   ```

2. **Validate JSON syntax**:
   - Use online validator
   - Or check with tools like `jq`:
     ```bash
     jq . data/temp_config.json
     ```

3. **Upload to ESP32**:
   ```bash
   pio run --target uploadfs
   ```

4. **Restart ESP32**

### Benefits

- Batch configure multiple devices
- Version control configurations
- Automated deployment scripts
- No web interface needed

---

## Migration from NVS

If you have an older version using NVS (Preferences):

### What Changed

| Before (NVS) | After (JSON) |
|--------------|--------------|
| Stored in NVS partition | Stored in LittleFS |
| Binary format | Human-readable JSON |
| Not easily readable | Can view/edit in text editor |
| Single namespace | Two separate files |

### Migration Steps

1. **Note your current settings** (write them down or screenshot)
2. **Upload new firmware**:
   ```bash
   pio run --target upload
   ```
3. **Edit JSON files** with your settings
4. **Upload filesystem**:
   ```bash
   pio run --target uploadfs
   ```
5. Settings restored!

**Note**: Old NVS data is not automatically migrated. You must manually set values.

---

## Best Practices

### ✅ Do

- Keep `data/` folder in version control
- Backup JSON files before major changes
- Use meaningful hostnames
- Validate JSON before uploading
- Document custom configurations

### ❌ Don't

- Don't commit WiFi passwords to public repositories
- Don't use special characters in hostnames
- Don't manually edit files on ESP32 (edit in `data/` then upload)
- Don't forget to restart after WiFi changes

---

## Example Configurations

### Home Setup

**wifi_config.json**:
```json
{
  "ssid": "HomeNetwork",
  "password": "SecurePassword123",
  "hostname": "spaHeater"
}
```

**temp_config.json**:
```json
{
  "tempDifferenceThreshold": 5.0,
  "minPanelTemp": 25.0,
  "maxSpaTemp": 40.0,
  "manualOverride": false,
  "pumpState": false
}
```

### Summer Configuration (More Aggressive)

```json
{
  "tempDifferenceThreshold": 3.0,
  "minPanelTemp": 20.0,
  "maxSpaTemp": 38.0,
  "manualOverride": false,
  "pumpState": false
}
```

### Winter Configuration (Conservative)

```json
{
  "tempDifferenceThreshold": 8.0,
  "minPanelTemp": 30.0,
  "maxSpaTemp": 42.0,
  "manualOverride": false,
  "pumpState": false
}
```

### Testing Setup

```json
{
  "tempDifferenceThreshold": 1.0,
  "minPanelTemp": 10.0,
  "maxSpaTemp": 50.0,
  "manualOverride": false,
  "pumpState": false
}
```

---

## API Reference

### ConfigManager Methods

#### Loading

```cpp
// Load temperature config from JSON
bool loadTempConfig(TempConfig& config);

// Load WiFi config from JSON
bool loadWiFiConfig(WiFiConfig& config);

// Load all configuration
bool loadAll(SpaConfig& config);
```

#### Saving

```cpp
// Save complete temperature config
bool saveTempConfig(const TempConfig& config);

// Save only temp parameters (preserve pump state)
bool saveTempParams(const TempConfig& config);

// Save only pump state (preserve temp parameters)
bool savePumpState(const TempConfig& config);

// Save WiFi configuration
bool saveWiFiConfig(const WiFiConfig& config);

// Save all configuration
bool saveAll(const SpaConfig& config);
```

#### Other

```cpp
// Reset to defaults (deletes and recreates JSON files)
bool reset();

// Print configuration to serial monitor
void printConfig(const SpaConfig& config);
```

---

## Summary

| Feature | Implementation |
|---------|----------------|
| **Storage** | LittleFS JSON files |
| **Temperature config** | `temp_config.json` |
| **WiFi config** | `wifi_config.json` |
| **Access by name** | mDNS: `chauffeSpa.local` |
| **Backup** | Copy `data/*.json` files |
| **Editing** | Web interface or direct file edit |
| **Reset** | Web interface or re-upload defaults |

---

**Version**: 2.1  
**Last updated**: 2026-04-13  
**Configuration format**: JSON (ArduinoJson v7)
