#include "logger.h"
#include "time_utils.h"
#include <stdarg.h>
#include <inttypes.h>

// Global logger instance
Logger logger;

Logger::Logger() {
  logs.reserve(MAX_LOGS);
}

const char* Logger::getLevelString(LogLevel level) {
  switch (level) {
    case LOG_INFO:    return "INFO";
    case LOG_WARNING: return "WARN";
    case LOG_ERROR:   return "ERROR";
    case LOG_SUCCESS: return "OK";
    default:          return "????";
  }
}

const char* Logger::getLevelIcon(LogLevel level) {
  switch (level) {
    case LOG_INFO:    return "ℹ️";
    case LOG_WARNING: return "⚠️";
    case LOG_ERROR:   return "❌";
    case LOG_SUCCESS: return "✅";
    default:          return "•";
  }
}

void Logger::log(LogLevel level, const String& message) {
  // Add to buffer
  uint64_t timestamp = getEpochMillis();

  // Remove oldest if at max capacity
  if (logs.size() >= MAX_LOGS) {
    logs.erase(logs.begin());
  }

  logs.push_back(LogEntry(timestamp, level, message));

  // Also print to serial
  char timeStr[32];
  if (!formatLocalTime(timestamp, timeStr, sizeof(timeStr))) {
    unsigned long fallbackMillis = millis();
    unsigned long seconds = fallbackMillis / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;

    snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu:%02lu",
             hours % 24, minutes % 60, seconds % 60);
  }

  Serial.printf("[%s] %s %s\n",
                timeStr,
                getLevelString(level),
                message.c_str());
}

void Logger::info(const String& message) {
  log(LOG_INFO, message);
}

void Logger::warning(const String& message) {
  log(LOG_WARNING, message);
}

void Logger::error(const String& message) {
  log(LOG_ERROR, message);
}

void Logger::success(const String& message) {
  log(LOG_SUCCESS, message);
}

void Logger::infof(const char* format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  info(String(buffer));
}

void Logger::warningf(const char* format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  warning(String(buffer));
}

void Logger::errorf(const char* format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  error(String(buffer));
}

void Logger::successf(const char* format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  success(String(buffer));
}

String Logger::getLogsJSON(int maxEntries) {
  String json = "[";

  int startIdx = 0;
  if (logs.size() > maxEntries) {
    startIdx = logs.size() - maxEntries;
  }

  for (size_t i = startIdx; i < logs.size(); i++) {
    if (i > startIdx) json += ",";

    const LogEntry& entry = logs[i];

    char timeStr[32];
    if (!formatLocalTime(entry.timestamp, timeStr, sizeof(timeStr))) {
      unsigned long seconds = entry.timestamp / 1000;
      unsigned long minutes = seconds / 60;
      unsigned long hours = minutes / 60;
      snprintf(timeStr, sizeof(timeStr), "%02lu:%02lu:%02lu",
               hours % 24, minutes % 60, seconds % 60);
    }

    char timestampStr[32];
    snprintf(timestampStr, sizeof(timestampStr), "%" PRIu64, entry.timestamp);

    json += "{";
    json += "\"timestamp\":" + String(timestampStr) + ",";
    json += "\"time\":\"" + String(timeStr) + "\",";
    json += "\"level\":\"" + String(getLevelString(entry.level)) + "\",";
    json += "\"icon\":\"" + String(getLevelIcon(entry.level)) + "\",";

    // Escape message for JSON
    String escapedMsg = entry.message;
    escapedMsg.replace("\\", "\\\\");
    escapedMsg.replace("\"", "\\\"");
    escapedMsg.replace("\n", "\\n");

    json += "\"message\":\"" + escapedMsg + "\"";
    json += "}";
  }

  json += "]";
  return json;
}

void Logger::clear() {
  logs.clear();
  info("Log cleared");
}
