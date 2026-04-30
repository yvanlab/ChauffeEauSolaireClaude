# RAM Usage Monitoring

## Feature Overview

The web interface now displays real-time RAM (heap) usage monitoring in the System tab, providing detailed insights into memory consumption.

## What's Displayed

### 🧠 RAM Usage Bar

**Visual Progress Bar:**
- Green (0-50%): Healthy memory usage
- Yellow (50-75%): Moderate usage
- Orange (75-90%): High usage
- Red (90-100%): Critical - may cause instability

**Main Display:**
- Used RAM / Total RAM (e.g., "46.2 KB / 320 KB")
- Percentage of RAM in use (e.g., "14.1%")

### 📊 Detailed Statistics

**Minimum Free Heap:**
- Lowest amount of free RAM ever reached since boot
- Important indicator of memory pressure
- If this drops too low (<50 KB), the system may become unstable

**Max Allocation:**
- Largest single memory block that can be allocated
- Useful for understanding memory fragmentation
- Should ideally be close to free heap

**History Buffer:**
- Shows exactly how much RAM the temperature graph uses
- Constant value: 23,040 bytes (22.5 KB) for 24-hour history
- Allocated at startup, never changes

## Memory Breakdown

### Typical ESP32 Memory Usage (v2.4)

```
Total ESP32 RAM: 320 KB (327,680 bytes)

┌─────────────────────────────────────┐
│ Used: ~46 KB (14%)                  │
│  ├─ History Buffer:    23 KB (50%)  │  ⬅ Largest component
│  ├─ WiFi/Network:      10 KB (22%)  │
│  ├─ Web Server:         8 KB (17%)  │
│  └─ Other/Stack:        5 KB (11%)  │
│                                     │
│ Free: ~275 KB (86%)                 │  ⬅ Plenty of headroom!
└─────────────────────────────────────┘
```

## Understanding the Values

### Used Heap
- RAM currently allocated and in use
- Includes all active buffers, variables, and objects
- Updates in real-time (every 10 seconds)

### Free Heap
- RAM available for new allocations
- Should remain comfortably above 100 KB
- If it drops below 50 KB, consider reducing history buffer

### Minimum Free Heap
- "Low water mark" - lowest free RAM since boot
- If much lower than current free heap, indicates temporary spikes
- Watch this value over days/weeks of operation

### Max Alloc Heap
- Largest contiguous block available
- Should be close to free heap
- Large difference indicates fragmentation

### History Buffer
- Fixed allocation: 1440 points × 16 bytes = 23,040 bytes
- Allocated once at startup (in constructor)
- Never grows or shrinks
- Only cleared on reboot

## Monitoring Best Practices

### Healthy System Indicators ✅

- **Free Heap:** >200 KB
- **Min Free Heap:** >150 KB
- **Heap Usage:** <25%
- **Max Alloc:** Close to Free Heap

### Warning Signs ⚠️

- **Free Heap:** <100 KB (may cause issues)
- **Min Free Heap:** <50 KB (risk of crashes)
- **Heap Usage:** >50% (monitor closely)
- **Max Alloc:** Much less than Free Heap (fragmentation)

### Critical Levels 🚨

- **Free Heap:** <50 KB (imminent crash risk)
- **Min Free Heap:** <20 KB (system unstable)
- **Heap Usage:** >80% (reduce features)

## Optimization Options

If you need to free up RAM:

### Option 1: Reduce History Duration
```cpp
// In include/webserver.h
#define MAX_HISTORY_POINTS 720  // 12 hours instead of 24
// Saves: 11.5 KB
```

### Option 2: Increase Sample Interval
```cpp
// In include/webserver.h
#define MAX_HISTORY_POINTS 720   // Same 24h coverage
#define HISTORY_INTERVAL_MS 120000  // 2 minutes instead of 1
// Saves: 11.5 KB
```

### Option 3: Shorter History + Longer Interval
```cpp
// In include/webserver.h
#define MAX_HISTORY_POINTS 360   // 12 hours at 2-minute intervals
#define HISTORY_INTERVAL_MS 120000  // 2 minutes
// Saves: 17.3 KB
```

## Auto-Refresh

The RAM statistics update automatically every **10 seconds** along with other dynamic system information.

## Troubleshooting

### High RAM Usage

**If heap usage is unexpectedly high:**

1. Check serial monitor for memory leaks
2. Review recent code changes
3. Verify no infinite loops creating objects
4. Check if WiFi is repeatedly reconnecting
5. Monitor over time to see if it grows

### Memory Fragmentation

**If Max Alloc << Free Heap:**

- Caused by many small allocations/deallocations
- Usually not a problem if free heap is still high
- ESP32 heap is fairly resistant to fragmentation
- Reboot clears fragmentation (starts fresh)

### Crashes or Resets

**If system reboots unexpectedly:**

1. Check Min Free Heap - if <20 KB, you're out of memory
2. Enable debug logging in serial monitor
3. Reduce history buffer size as temporary fix
4. Check for recursive functions or large stack usage

## Technical Details

### Data Source

All values come from ESP32 SDK functions:
```cpp
ESP.getFreeHeap()      // Current free RAM
ESP.getHeapSize()      // Total RAM available
ESP.getMinFreeHeap()   // Lowest free RAM since boot
ESP.getMaxAllocHeap()  // Largest block available
```

### Update Frequency

- Initial load: On page open
- Auto-refresh: Every 10 seconds
- No manual refresh needed

### History Buffer Allocation

```cpp
// Constructor allocates once at boot
WebServerManager::WebServerManager(...) {
  historyBuffer = new TempDataPoint[MAX_HISTORY_POINTS];
  // 1440 points × 16 bytes = 23,040 bytes allocated
}

// Destructor frees on shutdown (rare)
WebServerManager::~WebServerManager() {
  delete[] historyBuffer;
}
```

## Benefits

✅ **Real-time visibility** into memory usage  
✅ **Early warning** of memory issues  
✅ **Helps with debugging** memory-related crashes  
✅ **Validates** that history buffer size is appropriate  
✅ **Peace of mind** seeing 86% free RAM  

## Related Documentation

- `CHANGELOG_GRAPH.md` - Temperature graph feature details
- `README.md` - Main system documentation
- ESP32 Arduino Core docs - Memory management

---

**Added:** Version 2.4  
**Date:** 2026-04-20  
**Author:** Claude + ylabrit
