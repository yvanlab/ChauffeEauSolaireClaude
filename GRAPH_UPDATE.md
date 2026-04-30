# Graph Update - Real Clock Hours

## What Changed?

The temperature graph X-axis has been updated to show **real clock hours** instead of relative hours.

### Before (Relative Time)
```
X-axis: -24h, -20h, -16h, -12h, -8h, -4h, 0h (now)
```
- Showed hours relative to current moment
- "0h" meant "right now"
- "-12h" meant "12 hours ago"

### After (Real Clock Time)
```
X-axis: 08:00, 12:00, 16:00, 20:00, 00:00, 04:00, 08:00
```
- Shows actual clock time (e.g., 14:30, 18:45)
- Automatically uses your browser's time zone
- Easier to correlate with daily activities
- Tooltip shows full date and time when hovering

## Why This Is Better

✅ **More intuitive** - See exactly when temperatures peaked  
✅ **Easier correlation** - "Ah yes, the sun was strongest at 13:00"  
✅ **Real-world context** - Know what was happening at that time  
✅ **Time zone aware** - Automatically adjusts to your location  

## Technical Details

**Changed Components:**
1. Chart.js time scale instead of linear scale
2. Added chartjs-adapter-date-fns library for date handling
3. X-axis now uses JavaScript Date objects
4. Format: HH:mm (24-hour format)

**Libraries Added:**
- Chart.js 4.4.1 (already present)
- chartjs-adapter-date-fns 3.0.0 (new)

**File Changes:**
- `data/index.html` - Updated chart configuration and added date adapter

## Example Display

```
┌─────────────────────────────────────────────┐
│  50°C ┤                    ╭─╮               │
│  45°C ┤                 ╭──╯ ╰──╮            │  Panel
│  40°C ┤              ╭──╯       ╰─╮          │  (Yellow)
│  35°C ┤         ╭────╯            ╰──╮       │
│  30°C ┤   ╭─────╯                   ╰───╮   │  Spa
│  25°C ┤───╯                             ╰── │  (Blue)
│  20°C ┤                                     │  Air
│  15°C └─────────────────────────────────────│  (Red)
│         08:00  12:00  16:00  20:00  00:00   │
└─────────────────────────────────────────────┘
```

At a glance you can see:
- Morning (08:00): Cool panels, stable spa
- Midday (12:00): Panels heating up rapidly
- Afternoon (13:00): Peak panel temperature ~48°C
- Evening (18:00): Panels cooling, spa at maximum warmth
- Night (00:00): Everything cooling down

## Deployment

To update your ESP32:

```bash
cd C:\dev\ChauffeEauSolaireClaude

# Upload updated HTML interface
pio run --target uploadfs

# No firmware upload needed (backend unchanged)
```

## Testing with Simulator

The simulator already generates realistic 24-hour data with proper timestamps:

```bash
launch_simulator.bat
```

Open `http://localhost:8080` and click "Graphique 24h" to see the new time axis.

## Browser Compatibility

✅ Chrome/Edge - Full support  
✅ Firefox - Full support  
✅ Safari - Full support  
✅ Mobile browsers - Full support  

All modern browsers support the Date API and will display times correctly in your local time zone.

## Tooltip Format

When hovering over any point on the graph:

```
Air: 22.5°C
20/04 14:30
```

Shows:
- Temperature value with sensor name
- Date (DD/MM format)
- Time (HH:mm 24-hour format)

---

**Update Date**: 2026-04-20  
**Version**: 2.4 (revised)  
**Impact**: HTML only (no firmware changes required)
