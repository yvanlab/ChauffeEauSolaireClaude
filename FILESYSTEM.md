# Filesystem Management (LittleFS) - Guide

## Overview

This project uses **LittleFS** (Little File System) to store the HTML web interface and potentially other static files on the ESP32's flash memory. This approach offers several advantages:

✅ **Cleaner code** - HTML is separated from C++ code  
✅ **Easier editing** - Modify HTML without recompiling  
✅ **Smaller binary** - HTML isn't compiled into the program  
✅ **Faster updates** - Update web interface independently  

---

## Project Structure

```
ChauffeEauSolaireClaude/
├── data/                      # ← Filesystem files
│   └── index.html            # Main web interface
├── src/                      # Source code
├── include/                  # Headers
└── platformio.ini            # Configuration (LittleFS enabled)
```

All files in the `data/` folder will be uploaded to the ESP32's LittleFS partition.

---

## Uploading the Filesystem

### Method 1: Using PlatformIO CLI (Recommended)

```bash
# Navigate to project directory
cd C:\dev\ChauffeEauSolaireClaude

# Upload filesystem to ESP32
pio run --target uploadfs
```

**What happens:**
1. PlatformIO reads all files from the `data/` folder
2. Creates a LittleFS image
3. Uploads it to the ESP32's filesystem partition

### Method 2: Using PlatformIO IDE (VS Code)

1. Open the project in VS Code with PlatformIO extension
2. Click on the **PlatformIO icon** in the left sidebar
3. Expand **PROJECT TASKS** → **esp32dev**
4. Click **Platform** → **Upload Filesystem Image**

### Method 3: Using Arduino IDE (if applicable)

1. Install the **ESP32 LittleFS Uploader** plugin
2. Tools → ESP32 Sketch Data Upload

---

## Complete Upload Workflow

When developing, you need to upload **TWO** things separately:

### 1️⃣ Upload Program Code
```bash
pio run --target upload
```
Uploads the compiled C++ code (`main.cpp`, `config.cpp`, `webserver.cpp`, etc.)

### 2️⃣ Upload Filesystem
```bash
pio run --target uploadfs
```
Uploads the web interface (`data/index.html`)

**Important**: These are **independent** operations!
- Uploading code does NOT upload the filesystem
- Uploading filesystem does NOT upload the code

---

## First-Time Setup Checklist

When setting up the ESP32 for the first time:

```bash
# Step 1: Upload the program code
pio run --target upload

# Step 2: Upload the filesystem
pio run --target uploadfs

# Step 3: Monitor serial output
pio device monitor
```

Expected output:
```
✓ LittleFS mounted successfully
✓ HTTP server started on port 80
```

If you see:
```
✗ LittleFS mount failed!
  Make sure to upload filesystem with: pio run --target uploadfs
```
➜ You forgot step 2! Run `pio run --target uploadfs`

---

## Modifying the Web Interface

### Quick Edit Workflow

1. **Edit the HTML file**
   ```bash
   # Open with your favorite editor
   code data/index.html
   # or
   notepad data/index.html
   ```

2. **Upload only the filesystem** (no need to recompile code!)
   ```bash
   pio run --target uploadfs
   ```

3. **Refresh your browser** (Ctrl+F5 to clear cache)

4. **Done!** Changes are live immediately.

### What You Can Modify

**data/index.html** contains:
- All HTML structure
- CSS styling (embedded in `<style>` tag)
- JavaScript (embedded in `<script>` tag)

**Examples**:
- Change colors, fonts, layout
- Modify text labels (French → English)
- Add new UI elements
- Adjust refresh rate (currently 2 seconds)
- Add charts/graphs

---

## Filesystem Space

### Partition Size

By default, ESP32 boards have:
- **Program space**: ~1.2 MB
- **Filesystem space**: ~1.5 MB (LittleFS partition)

### Current Usage

| File | Size |
|------|------|
| `index.html` | ~14 KB |
| **Total** | **~14 KB / 1.5 MB** (< 1% used) |

You have **plenty of space** for:
- Additional HTML pages
- CSS files
- JavaScript libraries
- Images (small logos, icons)
- JSON configuration files

---

## Adding More Files

### Example: Adding a Logo

1. **Add file to data folder**
   ```bash
   # Copy logo to data folder
   cp logo.png data/
   ```

2. **Upload filesystem**
   ```bash
   pio run --target uploadfs
   ```

3. **Reference in HTML**
   ```html
   <img src="/logo.png" alt="Logo">
   ```

4. **Serve from ESP32** (add to `webserver.cpp`)
   ```cpp
   // In WebServerManager::begin()
   server->serveStatic("/logo.png", LittleFS, "/logo.png");
   ```

### Example: Separate CSS File

**data/style.css:**
```css
body {
  font-family: Arial, sans-serif;
  background: #667eea;
}
```

**data/index.html:**
```html
<link rel="stylesheet" href="/style.css">
```

**webserver.cpp:**
```cpp
server->serveStatic("/style.css", LittleFS, "/style.css");
```

---

## Troubleshooting

### ❌ "LittleFS mount failed"

**Cause**: Filesystem was not uploaded or is corrupted.

**Solution**:
```bash
pio run --target uploadfs
```

### ❌ "Error: index.html not found"

**Cause**: File is missing from the `data/` folder or wasn't uploaded.

**Solution**:
1. Verify file exists: `dir data\index.html` (Windows) or `ls data/index.html` (Linux/Mac)
2. Re-upload: `pio run --target uploadfs`

### ❌ "Upload failed" / "Timed out"

**Possible causes**:
- ESP32 not connected
- Wrong COM port
- Code running blocks upload (e.g., tight loop)

**Solutions**:
1. Check connection: `pio device list`
2. Hold BOOT button during upload
3. Reset ESP32 and try again
4. Check `platformio.ini` for correct `upload_port`

### ❌ "No space left on device"

**Cause**: LittleFS partition is full (rare with our small files).

**Solution**:
1. Delete unused files from `data/`
2. Consider custom partition scheme in `platformio.ini`:
   ```ini
   board_build.partitions = custom_partitions.csv
   ```

### ❌ Web page loads but shows old version

**Cause**: Browser cache.

**Solution**:
- **Chrome/Edge**: Ctrl + Shift + R
- **Firefox**: Ctrl + F5
- Or open in incognito/private mode

---

## Advanced: Custom Partition Table

If you need more filesystem space:

**1. Create `partitions.csv`:**
```csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x5000
otadata,  data, ota,     0xe000,  0x2000
app0,     app,  ota_0,   0x10000, 0x140000
app1,     app,  ota_1,   0x150000,0x140000
spiffs,   data, spiffs,  0x290000,0x170000
```

**2. Update `platformio.ini`:**
```ini
board_build.partitions = partitions.csv
```

**3. Re-upload everything:**
```bash
pio run --target upload
pio run --target uploadfs
```

---

## Comparison: Before vs After

### Before (Raw String in Code)

**Drawbacks**:
- HTML embedded in C++ as huge string literal
- Every HTML change requires full recompilation (~30 sec)
- Code file is 600+ lines
- Harder to maintain
- Wastes program flash space

### After (LittleFS)

**Benefits**:
- HTML in separate file (`data/index.html`)
- HTML changes: just upload filesystem (~5 sec)
- Code file reduced by ~500 lines
- Easy to edit with HTML editors
- Saves program flash space
- Professional separation of concerns

---

## Best Practices

✅ **Version Control**: Commit the entire `data/` folder to git  
✅ **Backups**: Keep local copies of `index.html` before major changes  
✅ **Testing**: Always test filesystem upload on a dev board first  
✅ **Comments**: Document custom modifications in HTML comments  
✅ **Minification**: For production, consider minifying HTML/CSS/JS  

---

## Filesystem API Reference

### In Code (main.cpp, webserver.cpp)

```cpp
#include <LittleFS.h>

// Initialize filesystem
bool success = LittleFS.begin(true);  // true = format on fail

// Check if file exists
bool exists = LittleFS.exists("/index.html");

// Open file for reading
File file = LittleFS.open("/index.html", "r");
String content = file.readString();
file.close();

// Open file for writing
File file = LittleFS.open("/log.txt", "w");
file.println("Log entry");
file.close();

// Delete file
LittleFS.remove("/old_file.html");

// Get filesystem info
size_t totalBytes = LittleFS.totalBytes();
size_t usedBytes = LittleFS.usedBytes();
```

### Serve Static Files (webserver.cpp)

```cpp
// Serve single file
request->send(LittleFS, "/index.html", "text/html");

// Serve entire directory
server->serveStatic("/", LittleFS, "/");

// Serve with cache control
server->serveStatic("/style.css", LittleFS, "/style.css")
      .setCacheControl("max-age=600");
```

---

## Summary

| Command | Purpose |
|---------|---------|
| `pio run --target upload` | Upload C++ code |
| `pio run --target uploadfs` | Upload filesystem (HTML) |
| `pio device monitor` | View serial output |

**Remember**: Upload filesystem at least once, then only when HTML changes!

---

**Last updated**: 2026-04-13  
**ESP32 Filesystem**: LittleFS (recommended over SPIFFS for ESP32)
