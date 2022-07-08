#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_LOGGING_H
#define HTFH_RT_SEARCH_CACHE_LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <time.h>

#ifndef ENABLE_LOGGING
#define LOG(level, stream, msg, ...) ({})
#else

#define CHECK_TYPE(type,var) { typedef void (*type_t)(type); type_t tmp = (type_t)0; if(0) tmp(var);}

enum LogLevel {
    ERROR = 0,
    WARN = 1,
    INFO = 2,
    DEBUG = 3,
    TRACE = 4
};

static int __min_log_level__ = DEBUG;

static inline char* logLevelToString(int level) {
    if (level == ERROR)  {
        return "ERROR";
    } else if (level == WARN) {
        return "WARN ";
    } else if (level == INFO) {
        return "INFO ";
    } else if (level == DEBUG) {
        return "DEBUG";
    } else if (level == TRACE) {
        return "TRACE";
    }
    return "INFO ";
}

// Log streams
#define STDOUT stdout
#define STDERR stderr

#define LOG(level, stream, msg, ...) ({ \
    CHECK_TYPE(int, level); \
    if (level <= __min_log_level__) {   \
        time_t rawtime; \
        struct tm* timeinfo; \
        time(&rawtime); \
        timeinfo = localtime(&rawtime); \
        fprintf(stream, "[%d/%d/%d %d:%d:%d] %s:%d [%s] :: " msg "\n", \
            timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, \
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, \
            __func__, __LINE__, \
            logLevelToString(level), \
            ##__VA_ARGS__ \
        ); \
    } \
})

#endif // ENABLE_LOGGING

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_LOGGING_H
