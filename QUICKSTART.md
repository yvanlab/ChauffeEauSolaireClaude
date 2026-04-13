# Quick Start Guide - Chauffage Solaire Spa

**Get your ESP32 solar spa controller running in 5 minutes!**

---

## 📋 Prerequisites

- ✅ ESP32 Development Board
- ✅ 3x DS18B20 temperature sensors
- ✅ 1x Relay module
- ✅ USB cable
- ✅ PlatformIO installed
- ✅ Hardware wired (see `WIRING.txt`)

---

## 🚀 Step-by-Step Setup

### Step 1: Configure WiFi (2 minutes)

Open `data/wifi_config.json` and edit:

```json
{
  "ssid": "YourWiFiNetwork",
  "password": "YourWiFiPassword",
  "hostname": "chauffeSpa"
}
```

**That's it!** Default temperature settings are fine to start.

---

### Step 2: Upload to ESP32 (2 minutes)

Open terminal in project folder:

```bash
cd C:\dev\ChauffeEauSolaireClaude

# Upload program code
pio run --target upload

# Upload filesystem (HTML + JSON configs)
pio run --target uploadfs

# Watch the boot process
pio device monitor
```

---

### Step 3: Access Web Interface (1 minute)

**Option A: By hostname** (easiest)
```
http://chauffeSpa.local
```

**Option B: By IP address**
```
Check serial monitor for IP, then:
http://192.168.1.xxx
```

---

## ✅ First Time Checklist

In the web interface:

1. **Verify temperatures** are reading correctly:
   - Air temperature
   - Spa water temperature
   - Solar panel water temperature

2. **Test pump control**:
   - Click "Marche forcée" → pump should turn ON
   - Click "Arrêt forcé" → pump should turn OFF
   - Click "Mode automatique" → pump controlled by temperatures

3. **Configure thresholds** (Configuration tab):
   - Temperature difference: How much warmer panel must be than spa (default: 5°C)
   - Min panel temp: Minimum panel temperature to activate (default: 25°C)
   - Max spa temp: Safety limit for spa (default: 40°C)

---

## 🎯 What It Does

**Automatic Mode** (default):
- Monitors temperatures every 2 seconds
- When panel is warmer than spa + threshold: **Pump ON**
- When temperature difference drops: **Pump OFF**
- Safety: Always stops if spa reaches max temperature

**Manual Mode**:
- Override automatic control
- Force pump ON or OFF
- State saved and restored after power loss

---

## 📊 Serial Monitor Output

You should see something like:

```
╔════════════════════════════════════════╗
║     SYSTEM READY                       ║
╚════════════════════════════════════════╝
Web interface: http://192.168.1.100
           or: http://chauffeSpa.local
════════════════════════════════════════

Air: 22.5°C | Spa: 30.2°C | Panel: 38.7°C | Diff: 8.5°C | Pump: ON  | Mode: AUTO
```

---

## 🔧 Common Issues

### ❌ "LittleFS mount failed"

**Solution**: You forgot to upload the filesystem!
```bash
pio run --target uploadfs
```

---

### ❌ "WiFi connection failed"

**Solution**: Check your credentials in `data/wifi_config.json`
- Make sure SSID is correct (case-sensitive)
- Make sure password is correct
- ESP32 only supports 2.4 GHz WiFi (not 5 GHz)

Re-upload after fixing:
```bash
pio run --target uploadfs
```

---

### ❌ "Only 0 sensors detected"

**Solution**: Check your wiring
- Verify GPIO4 connection
- Check 4.7kΩ pull-up resistor between DATA and 3.3V
- Verify all sensors have power (3.3V and GND)

See `WIRING.txt` for detailed diagrams.

---

### ❌ "http://chauffeSpa.local doesn't work"

**Solution 1**: Use IP address instead (always works)

**Solution 2**: Windows users may need Bonjour
- Download: Bonjour Print Services from Apple
- Or just use the IP address shown in serial monitor

---

## 📖 Next Steps

### Basic Usage

- **Monitor**: Just open `http://chauffeSpa.local` in your browser
- **Configure**: Use the Configuration tab to adjust thresholds
- **Control**: Use manual buttons when needed

### Advanced

- **Customize HTML**: Edit `data/index.html` and upload with `pio run --target uploadfs`
- **Change hostname**: Edit `data/wifi_config.json` to use different name
- **Backup config**: Copy `data/*.json` files to safe location
- **Multiple devices**: Use different hostnames for each ESP32

---

## 📚 Full Documentation

For more details, see:

- **README.md** - Complete project documentation
- **JSON_CONFIG.md** - Configuration management & mDNS guide
- **FILESYSTEM.md** - LittleFS and file upload details
- **WIRING.txt** - Hardware connection diagrams
- **CHANGELOG_JSON_MDNS.md** - Recent changes and migration

---

## 🎓 Understanding the System

### Temperature Logic

```
IF panel_temp >= min_panel_temp (25°C)
   AND spa_temp < max_spa_temp (40°C)
   AND (panel_temp - spa_temp) >= threshold (5°C)
THEN
   Pump ON
ELSE
   Pump OFF
```

### Hysteresis

Once pump is ON, it only turns OFF when:
```
(panel_temp - spa_temp) < (threshold - 1°C)
```

This prevents rapid on/off cycling.

### Safety

If spa temperature reaches max (40°C):
- Pump FORCED OFF immediately
- Overrides all other conditions
- Prevents overheating

---

## 🆘 Getting Help

1. **Check serial monitor** for error messages
2. **Verify wiring** against `WIRING.txt`
3. **Test sensors individually** (disconnect two, test one)
4. **Check documentation** in project folder
5. **Reset to defaults**: Use web interface System tab

---

## 🎉 Success!

If you see:
- ✅ Three temperatures updating in web interface
- ✅ Pump indicator shows correct state
- ✅ Configuration saves properly
- ✅ System survives reboot

**You're all set!** The system will now automatically manage your spa heating.

---

## 📝 Quick Command Reference

```bash
# Complete setup (first time)
pio run --target upload && pio run --target uploadfs

# Update code only
pio run --target upload

# Update HTML/configs only
pio run --target uploadfs

# Monitor serial output
pio device monitor

# Clean and rebuild
pio run --target clean && pio run --target upload
```

---

## 🔑 Default Values

**Temperature Parameters**:
- Temperature difference threshold: **5.0°C**
- Minimum panel temperature: **25.0°C**
- Maximum spa temperature: **40.0°C**

**Network**:
- Hostname: **chauffeSpa** (`http://chauffeSpa.local`)
- Web server port: **80** (HTTP)

**Hardware**:
- DS18B20 sensors: **GPIO4**
- Relay (pump): **GPIO14**
- Serial baud rate: **115200**

---

**Version**: 2.1  
**Last updated**: 2026-04-13  
**Time to setup**: ~5 minutes  
**Difficulty**: Beginner-friendly
