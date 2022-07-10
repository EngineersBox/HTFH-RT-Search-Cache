#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_LOGGING_H
#define HTFH_RT_SEARCH_CACHE_LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <time.h>

#include "../types/typecheck.h"

enum LogLevel {
    LL_ERROR = 0,
    LL_WARN = 1,
    LL_INFO = 2,
    LL_DEBUG = 3,
    LL_TRACE = 4
};

static int __min_log_level__ = LL_TRACE;

#ifndef ENABLE_LOGGING
#define LOG(level, stream, msg, ...) ({})
#else

static inline char* logLevelToString(int level) {
    if (level == LL_ERROR)  {
        return "ERROR";
    } else if (level == LL_WARN) {
        return "WARN ";
    } else if (level == LL_INFO) {
        return "INFO ";
    } else if (level == LL_DEBUG) {
        return "DEBUG";
    } else if (level == LL_TRACE) {
        return "TRACE";
    }
    return "INFO ";
}

// Log streams
#define STDOUT stdout
#define STDERR stderr

#define __FILENAME__ (strrchr("/" __FILE__, '/') + 1)

#ifdef LOG_DATETIME_PREFIX
#define __GET_DATETIME_FORMAT_VALUES timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
#define __DATETIME_PREFIX "[%d/%d/%d %d:%d:%d] "
#define __DEFINE_DATETIME_STRUCTS time_t rawtime; time(&rawtime); struct tm* timeinfo = localtime(&rawtime);
#else
#define __GET_DATETIME_FORMAT_VALUES
#define __DATETIME_PREFIX ""
#define __DEFINE_DATETIME_STRUCTS ({});
#endif

#define LOG(level, msg, ...) { \
    if (typename(level) != T_INT) { \
        fprintf(STDERR, "Expected integer log level"); \
        exit(1); \
    } \
    if (level <= __min_log_level__) { \
        __DEFINE_DATETIME_STRUCTS; \
        fprintf(               \
            level == LL_ERROR ? STDERR : STDOUT, \
            __DATETIME_PREFIX "%s(%s:%d) [%s] :: " \
            msg "\n", \
            __GET_DATETIME_FORMAT_VALUES \
            __func__, __FILENAME__, __LINE__, \
            logLevelToString(level), \
            ##__VA_ARGS__ \
        ); \
        fflush(level == LL_ERROR ? STDERR : STDOUT); \
    } \
}

//#define LOG(level, msg, ...) printf(msg "\n", ##__VA_ARGS__)

#define ERROR(msg, ...) (LOG(LL_ERROR, msg, ##__VA_ARGS__))
#define WARN(msg, ...) (LOG(LL_WARN, msg, ##__VA_ARGS__))
#define INFO(msg, ...) (LOG(LL_INFO, msg, ##__VA_ARGS__))
#define DEBUG(msg, ...) (LOG(LL_DEBUG, msg, ##__VA_ARGS__))
#define TRACE(msg, ...) (LOG(LL_TRACE, msg, ##__VA_ARGS__))

#endif // ENABLE_LOGGING

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_LOGGING_H
