#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <stdint.h>
#include <stddef.h>

// Initialize NTP and local time.
// Returns true if time sync was successful.
bool initializeTime();

// Return the current epoch timestamp in milliseconds.
uint64_t getEpochMillis();

// Format a local time string from an epoch timestamp in milliseconds.
// Returns true if the timestamp could be converted.
bool formatLocalTime(uint64_t epochMillis, char* buffer, size_t bufferSize);

#endif // TIME_UTILS_H
