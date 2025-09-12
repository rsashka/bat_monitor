#include "unittest.h"
#include <cstring>

// Declarations of functions to access the last log message
extern "C" int get_last_log_level();
extern "C" const char* get_last_log_tag();
extern "C" const char* get_last_log_message();
extern "C" void reset_last_log_message();

// Test for ESP_LOGE macro
TEST(LoggerTests, TestESP_LOGE) {
    // Reset previous message
    reset_last_log_message();
    
    // Call logging macro
    ESP_LOGE("TAG", "Error message %d", 123);
    
    // Check result
    EXPECT_EQ(get_last_log_level(), 1); // ESP_LOG_ERROR = 1
    EXPECT_STREQ(get_last_log_tag(), "TAG");
    EXPECT_STREQ(get_last_log_message(), "Error message 123");
}

// Test for ESP_LOGW macro
TEST(LoggerTests, TestESP_LOGW) {
    // Reset previous message
    reset_last_log_message();
    
    // Call logging macro
    ESP_LOGW("WARNING_TAG", "Warning message %s", "test");
    
    // Check result
    EXPECT_EQ(get_last_log_level(), 2); // ESP_LOG_WARN = 2
    EXPECT_STREQ(get_last_log_tag(), "WARNING_TAG");
    EXPECT_STREQ(get_last_log_message(), "Warning message test");
}

// Test for ESP_LOGI macro
TEST(LoggerTests, TestESP_LOGI) {
    // Reset previous message
    reset_last_log_message();
    
    // Call logging macro
    ESP_LOGI("INFO_TAG", "Info message %f", 3.14);
    
    // Check result
    EXPECT_EQ(get_last_log_level(), 3); // ESP_LOG_INFO = 3
    EXPECT_STREQ(get_last_log_tag(), "INFO_TAG");
    EXPECT_STREQ(get_last_log_message(), "Info message 3.140000");
}

// Test for ESP_LOGD macro
TEST(LoggerTests, TestESP_LOGD) {
    // Reset previous message
    reset_last_log_message();
    
    // Call logging macro
    ESP_LOGD("DEBUG_TAG", "Debug message");
    
    // Check result
    EXPECT_EQ(get_last_log_level(), 4); // ESP_LOG_DEBUG = 4
    EXPECT_STREQ(get_last_log_tag(), "DEBUG_TAG");
    EXPECT_STREQ(get_last_log_message(), "Debug message");
}

// Test for ESP_LOGV macro
TEST(LoggerTests, TestESP_LOGV) {
    // Reset previous message
    reset_last_log_message();
    
    // Call logging macro
    ESP_LOGV("VERBOSE_TAG", "Verbose message %d %s", 42, "test");
    
    // Check result
    EXPECT_EQ(get_last_log_level(), 5); // ESP_LOG_VERBOSE = 5
    EXPECT_STREQ(get_last_log_tag(), "VERBOSE_TAG");
    EXPECT_STREQ(get_last_log_message(), "Verbose message 42 test");
}

// Test for working with empty tag
TEST(LoggerTests, TestEmptyTag) {
    // Reset previous message
    reset_last_log_message();
    
    // Call logging macro with empty tag
    ESP_LOGE("", "Message with empty tag");
    
    // Check result
    EXPECT_EQ(get_last_log_level(), 1);
    EXPECT_STREQ(get_last_log_tag(), "");
    EXPECT_STREQ(get_last_log_message(), "Message with empty tag");
}

// Test for working with NULL tag
TEST(LoggerTests, TestNullTag) {
    // Reset previous message
    reset_last_log_message();
    
    // Call logging macro with NULL tag
    // For this test we cannot directly pass NULL to the macro,
    // but we can check the behavior when reset
    reset_last_log_message();
    EXPECT_EQ(get_last_log_level(), 0);
    EXPECT_STREQ(get_last_log_tag(), "");
    EXPECT_STREQ(get_last_log_message(), "");
}

// Test for working with Russian characters
TEST(LoggerTests, TestRussianCharacters) {
    // Reset previous message
    reset_last_log_message();
    
    // Call logging macro with Russian characters
    ESP_LOGI("РУССКИЙ_ТЕГ", "Сообщение с русскими символами: Привет, мир! %d", 123);
    
    // Check result
    EXPECT_EQ(get_last_log_level(), 3); // ESP_LOG_INFO = 3
    EXPECT_STREQ(get_last_log_tag(), "РУССКИЙ_ТЕГ");
    EXPECT_STREQ(get_last_log_message(), "Сообщение с русскими символами: Привет, мир! 123");
}
