# Changelog - Temperature Graph Feature

## Version 2.4 - 2026-04-20

### New Feature: 24-Hour Temperature Graph

Added a new tab in the web interface that displays a graph showing the evolution of all three temperature sensors over the last 24 hours.

#### What's New

1. **New "Graphique 24h" Tab**
   - Interactive line chart showing Air, Spa, and Panel temperatures
   - 24-hour historical view with automatic updates every 30 seconds
   - Color-coded lines: Red (Air), Blue (Spa), Yellow (Panels)
   - Hover to see exact temperature values at any point in time

2. **Backend Changes**
   - Added circular buffer to store temperature history (1440 data points max)
   - Records temperatures every minute automatically
   - New `/history` API endpoint to retrieve historical data as JSON
   - Memory-efficient storage using a ring buffer

3. **Frontend Changes**
   - Integrated Chart.js library for smooth, responsive graphs
   - Integrated chartjs-adapter-date-fns for time scale support
   - X-axis shows real clock time (e.g., 08:00, 12:00, 16:00)
   - Y-axis shows temperature in Celsius
   - Manual refresh button available
   - Displays total number of data points collected

#### Technical Details

**Memory Usage:**
- Each data point: 16 bytes (timestamp + 3 floats)
- Max buffer size: 1440 points × 16 bytes = ~23 KB
- Circular buffer prevents memory overflow

**Data Collection:**
- Interval: 60 seconds (1 minute)
- Retention: Last 24 hours (1440 minutes)
- Oldest data automatically replaced by newest

**API Endpoints:**
- `GET /history` - Returns JSON with all temperature history points
  ```json
  {
    "points": [
      {"t": 1234567890, "a": 22.5, "s": 30.2, "p": 38.7},
      ...
    ],
    "count": 145
  }
  ```

#### Files Modified

1. **include/webserver.h**
   - Added `TempDataPoint` struct for historical data
   - Added history buffer and management variables
   - Added `recordHistory()` and `handleHistory()` methods

2. **src/webserver.cpp**
   - Implemented circular buffer logic in constructor/destructor
   - Added `/history` route handler
   - Implemented `recordHistory()` to store data every minute
   - Implemented `handleHistory()` to serve JSON data

3. **src/main.cpp**
   - Added call to `webServer->recordHistory()` in main loop

4. **data/index.html**
   - Added Chart.js library (CDN)
   - Added new "Graphique 24h" tab with canvas element
   - Added chart styling and legend
   - Implemented JavaScript chart initialization and auto-refresh
   - Added manual refresh button

#### Usage

1. **Upload the updated filesystem** (required for HTML changes):
   ```bash
   pio run --target uploadfs
   ```

2. **Upload the firmware** (required for backend changes):
   ```bash
   pio run --target upload
   ```

3. **Access the graph:**
   - Open the web interface: `http://chauffeSpa.local` or `http://<IP>`
   - Click on the "Graphique 24h" tab
   - The graph will automatically load and refresh every 30 seconds
   - X-axis shows real clock time (updates automatically to current time zone)

#### Notes

- Data collection starts fresh after each reboot (no persistence to flash)
- First 24 hours will gradually fill the graph as data is collected
- Chart is responsive and works on mobile devices
- Minimal performance impact: only records once per minute
- X-axis shows real clock time in your local time zone (e.g., 08:00, 12:00, 16:00)
- Tooltip shows date and time when hovering over data points

#### Future Enhancements

Possible improvements for future versions:
- [ ] Persist history to LittleFS for survival across reboots
- [ ] Configurable data retention period (12h, 24h, 48h, 1 week)
- [ ] Export data as CSV for external analysis
- [ ] Add pump state overlay on the graph
- [ ] Date/time picker to view specific time ranges
- [ ] Min/max temperature markers
- [ ] Zoom and pan controls

---

**Version**: 2.4  
**Date**: 2026-04-20  
**Author**: Claude + ylabrit
