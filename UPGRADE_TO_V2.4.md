# Upgrade Guide: Version 2.3 → 2.4

## What's New in Version 2.4?

📊 **Temperature Graph Feature**  
A new "Graphique 24h" tab has been added to the web interface, displaying an interactive graph showing the evolution of all three temperature sensors over the last 24 hours with real clock time (08:00, 12:00, etc).

🧠 **RAM Usage Monitoring**  
Real-time RAM (heap) usage monitoring added to the System tab, showing detailed memory statistics including used/free RAM, minimum free heap, and history buffer size.

## Quick Start

### 1. Upload the Updated Filesystem (Required)

The HTML interface has been modified to include the new graph tab and Chart.js library.

```bash
cd C:\dev\ChauffeEauSolaireClaude
pio run --target uploadfs
```

**⚠️ Important**: This will overwrite your existing HTML interface. Any custom modifications will be lost.

### 2. Upload the Updated Firmware (Required)

The backend has been updated to record and serve temperature history.

```bash
pio run --target upload
```

### 3. Monitor the System (Optional)

```bash
pio device monitor
```

You should see version 2.4 in the startup banner.

## What to Expect

### Immediate After Upgrade

- The system will start with an empty graph
- Temperature data begins recording immediately (every 60 seconds)
- Graph will gradually fill over the next 24 hours

### First Hour

- Graph will show the last ~60 minutes of data
- X-axis will span from -1h to 0h (now)

### After 24 Hours

- Graph will show complete 24-hour history
- 1440 data points displayed (one per minute)
- Oldest data automatically replaced by newest

## Using the Graph

1. Open the web interface: `http://chauffeSpa.local` or `http://<IP>`
2. Click on the **"Graphique 24h"** tab
3. The graph loads automatically and refreshes every 30 seconds
4. Hover over lines to see exact temperatures
5. Click **"🔄 Actualiser le graphique"** to manually refresh

## Testing with the Simulator

To test the interface without hardware:

```bash
cd C:\dev\ChauffeEauSolaireClaude
python web_simulator.py
```

Then open: `http://localhost:8080`

The simulator includes pre-generated 24-hour history with realistic temperature patterns.

## Memory Impact

- **RAM Usage**: +23 KB (circular buffer for 1440 points)
- **Flash Usage**: No significant change
- **Performance**: Negligible (records once per minute)

Current memory usage after upgrade:
- RAM: 14.1% (46,192 bytes / 327,680 bytes)
- Flash: 68.9% (902,801 bytes / 1,310,720 bytes)

Still plenty of room for future features!

## Rollback (If Needed)

If you need to revert to version 2.3:

```bash
git checkout HEAD~1
pio run --target upload
pio run --target uploadfs
```

## Troubleshooting

### Graph not showing

- **Check browser console** (F12) for JavaScript errors
- **Verify Chart.js loaded**: Look for errors loading CDN
- **Clear browser cache**: Ctrl+F5
- **Check /history endpoint**: Visit `http://<IP>/history` directly

### No data in graph

- Wait a few minutes for first data points to be recorded
- Check that the ESP32 is running (LED blinking, serial output)
- Verify sensors are working (check main dashboard temperatures)

### Graph freezes or doesn't update

- Manual refresh using the button
- Check WiFi connection stability
- Check serial monitor for errors

## Additional Resources

- **Full changelog**: See `CHANGELOG_GRAPH.md`
- **Simulator guide**: See `SIMULATOR.md`
- **Main documentation**: See `README.md`

## Support

If you encounter issues:

1. Check serial monitor output: `pio device monitor`
2. Verify all sensors are connected and reading correctly
3. Ensure WiFi connection is stable
4. Check browser developer console for errors

---

**Upgrade from**: Version 2.3  
**Upgrade to**: Version 2.4  
**Date**: 2026-04-20  
**Estimated time**: ~5 minutes
