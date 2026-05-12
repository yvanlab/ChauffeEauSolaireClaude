#include "time_utils.h"
#include <Arduino.h>
#include <time.h>
#include <string.h>
#include <inttypes.h>

static bool timeSynced = false;
static uint64_t bootEpochMillis = 0;

bool initializeTime() {
  // configTzTime is the robust way on ESP32 to set both NTP and Timezone.
  // France uses Central European Time (CET/CEST):
  // - Standard (CET): UTC+1 (POSIX offset -1)
  // - Daylight (CEST): UTC+2
  // - Transition: Last Sunday of March (M3.5.0) to Last Sunday of October (M10.5.0)
  const char* tzConfig = "CET-1CEST,M3.5.0,M10.5.0/3";
  
  configTzTime(tzConfig, "pool.ntp.org", "fr.pool.ntp.org", "time.nist.gov");

  struct tm timeinfo;
  for (int attempt = 0; attempt < 10; attempt++) {
    if (getLocalTime(&timeinfo)) {
      timeSynced = true;
      uint64_t nowMs = (uint64_t)time(nullptr) * 1000ULL;
      bootEpochMillis = nowMs - millis();
      return true;
    }
    delay(1000);
  }

  return false;
}

uint64_t getEpochMillis() {
  time_t nowSeconds = time(nullptr);
  if (nowSeconds > 100000) {
    return (uint64_t)nowSeconds * 1000ULL;
  }

  if (bootEpochMillis != 0) {
    return bootEpochMillis + millis();
  }

  return millis();
}

bool formatLocalTime(uint64_t epochMillis, char* buffer, size_t bufferSize) {
  if (!timeSynced) {
    return false;
  }

  time_t seconds = epochMillis / 1000ULL;
  struct tm timeinfo;
  if (localtime_r(&seconds, &timeinfo) == NULL) {
    return false;
  }

  size_t len = strftime(buffer, bufferSize, "%Y-%m-%d %H:%M:%S", &timeinfo);
  return len > 0;
}
