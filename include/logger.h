#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <stdint.h>
#include <vector>

// Log levels
enum LogLevel {
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR,
  LOG_SUCCESS
};

// Log entry structure
struct LogEntry {
  uint64_t timestamp;  // epoch milliseconds or fallback uptime-based millis
  LogLevel level;
  String message;

  LogEntry(uint64_t ts, LogLevel lvl, const String& msg)
    : timestamp(ts), level(lvl), message(msg) {}
};

class Logger {
private:
  static const int MAX_LOGS = 20;  // Keep last 20 log entries
  std::vector<LogEntry> logs;

  const char* getLevelString(LogLevel level);
  const char* getLevelIcon(LogLevel level);

public:
  Logger();

  // Log methods
  void info(const String& message);
  void warning(const String& message);
  void error(const String& message);
  void success(const String& message);

  // Printf-style logging
  void infof(const char* format, ...);
  void warningf(const char* format, ...);
  void errorf(const char* format, ...);
  void successf(const char* format, ...);

  // Get logs
  const std::vector<LogEntry>& getLogs() const { return logs; }

  // Get logs as JSON
  String getLogsJSON(int maxEntries = 50);

  // Clear logs
  void clear();

  // Get log count
  int getCount() const { return logs.size(); }

private:
  void log(LogLevel level, const String& message);
};

// Global logger instance
extern Logger logger;

#endif // LOGGER_H
