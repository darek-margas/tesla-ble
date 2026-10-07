#pragma once

#ifndef TESLA_LOG_TAG
#define TESLA_LOG_TAG "TeslaBLE"
#endif

#include <cstdarg>

#ifdef ESP_PLATFORM
#include <esp_log.h>
#endif

namespace TeslaBLE {

enum class LogLevel { ERROR, WARN, INFO, DEBUG, VERBOSE };

using LogCallback = void (*)(LogLevel level, const char *tag, int line, const char *format, va_list args);

void set_log_callback(LogCallback callback);
LogCallback get_log_callback();

void log_internal(LogLevel level, const char *tag, int line, const char *format, ...)
    __attribute__((format(printf, 4, 5)));

}  // namespace TeslaBLE

// Compile-time log level: messages above it are compiled out, so their format
// strings do not take flash. 0 = ERROR, 1 = WARN, 2 = INFO, 3 = DEBUG,
// 4 = VERBOSE (default: keep everything; the runtime callback still filters).
#ifndef TESLA_BLE_LOG_LEVEL
#define TESLA_BLE_LOG_LEVEL 4
#endif

#define TESLA_BLE_LOG_AT_(min_level, level, format, ...) \
  do { \
    if (TESLA_BLE_LOG_LEVEL >= (min_level)) \
      TeslaBLE::log_internal(level, TESLA_LOG_TAG, __LINE__, format __VA_OPT__(, ) __VA_ARGS__); \
  } while (0)

#define LOG_ERROR(format, ...) TESLA_BLE_LOG_AT_(0, TeslaBLE::LogLevel::ERROR, format __VA_OPT__(, ) __VA_ARGS__)
#define LOG_WARNING(format, ...) TESLA_BLE_LOG_AT_(1, TeslaBLE::LogLevel::WARN, format __VA_OPT__(, ) __VA_ARGS__)
#define LOG_INFO(format, ...) TESLA_BLE_LOG_AT_(2, TeslaBLE::LogLevel::INFO, format __VA_OPT__(, ) __VA_ARGS__)
#define LOG_DEBUG(format, ...) TESLA_BLE_LOG_AT_(3, TeslaBLE::LogLevel::DEBUG, format __VA_OPT__(, ) __VA_ARGS__)
#define LOG_VERBOSE(format, ...) TESLA_BLE_LOG_AT_(4, TeslaBLE::LogLevel::VERBOSE, format __VA_OPT__(, ) __VA_ARGS__)
