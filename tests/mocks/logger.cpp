#include "unittest.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>

// Global variables to store last log message for testing
static int last_log_level = 0;
static char last_log_tag[256] = {0};
static char last_log_message[1024] = {0};

// Implementation of moc_logger function for unit tests
extern "C" int moc_logger(int loglevel, const char *tag, const char *fmt, ...) {
    // Store log level
    last_log_level = loglevel;
    
    // Store tag
    if (tag) {
        strncpy(last_log_tag, tag, sizeof(last_log_tag) - 1);
        last_log_tag[sizeof(last_log_tag) - 1] = '\0';
    } else {
        last_log_tag[0] = '\0';
    }
    
    // Format and store message
    if (fmt) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(last_log_message, sizeof(last_log_message), fmt, args);
        va_end(args);
    } else {
        last_log_message[0] = '\0';
    }
    
    // For debugging purposes, we could print to stdout
    // printf("LOG [%d] (%s): %s\n", loglevel, tag ? tag : "NULL", last_log_message);
    
    // Return 0 to indicate success (similar to ESP_LOG macros)
    return 0;
}

// Helper functions for unit tests to access last log message
extern "C" int get_last_log_level() {
    return last_log_level;
}

extern "C" const char* get_last_log_tag() {
    return last_log_tag;
}

extern "C" const char* get_last_log_message() {
    return last_log_message;
}

extern "C" void reset_last_log_message() {
    last_log_level = 0;
    last_log_tag[0] = '\0';
    last_log_message[0] = '\0';
}
