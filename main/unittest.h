#ifndef UNITTEST_H
#define UNITTEST_H

#ifndef BUILD_UNITTEST

// Logger from ESP32

#include "esp_err.h"
#include "esp_log.h"

#define SCOPE(name) name:

#else  // BUILD_UNITTEST

#define SCOPE(name) public:

#include <gtest/gtest.h>

// Logger mock for unit test
#ifndef MOCK_LOGGER

extern "C" int moc_logger(int loglevel, const char *tag, const char *fmt...);
#define MOCK_LOGGER moc_logger

#define MOCK_LOGLEVEL_ERROR 1
#define MOCK_LOGLEVEL_WARN 2
#define MOCK_LOGLEVEL_INFO 3
#define MOCK_LOGLEVEL_DEBUG 4
#define MOCK_LOGLEVEL_VERBOSE 5

#if defined(__cplusplus) && (__cplusplus > 201703L)
#define ESP_LOGE(tag, format, ...) MOCK_LOGGER(MOCK_LOGLEVEL_ERROR, tag, format __VA_OPT__(, ) __VA_ARGS__)
#define ESP_LOGW(tag, format, ...) MOCK_LOGGER(MOCK_LOGLEVEL_WARN, tag, format __VA_OPT__(, ) __VA_ARGS__)
#define ESP_LOGI(tag, format, ...) MOCK_LOGGER(MOCK_LOGLEVEL_INFO, tag, format __VA_OPT__(, ) __VA_ARGS__)
#define ESP_LOGD(tag, format, ...) MOCK_LOGGER(MOCK_LOGLEVEL_DEBUG, tag, format __VA_OPT__(, ) __VA_ARGS__)
#define ESP_LOGV(tag, format, ...) MOCK_LOGGER(MOCK_LOGLEVEL_VERBOSE, tag, format __VA_OPT__(, ) __VA_ARGS__)
#else  // !(defined(__cplusplus) && (__cplusplus >  201703L))
#define ESP_LOGE(tag, format, ...) MOCK_LOGGER(MOCK_LOGLEVEL_ERROR, tag, format, ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...) MOCK_LOGGER(MOCK_LOGLEVEL_WARN, tag, format, ##__VA_ARGS__)
#define ESP_LOGI(tag, format, ...) MOCK_LOGGER(MOCK_LOGLEVEL_INFO, tag, format, ##__VA_ARGS__)
#define ESP_LOGD(tag, format, ...) MOCK_LOGGER(MOCK_LOGLEVEL_DEBUG, tag, format, ##__VA_ARGS__)
#define ESP_LOGV(tag, format, ...) MOCK_LOGGER(MOCK_LOGLEVEL_VERBOSE, tag, format, ##__VA_ARGS__)
#endif  // !(defined(__cplusplus) && (__cplusplus >  201703L))

#endif  // MOCK_LOGGER

// Error check mock for unit test
#include "assert.h"
#define ESP_ERROR_CHECK(x) assert(ESP_OK == (x))

#endif  // BUILD_UNITTEST

#endif  // UNITTEST_H
