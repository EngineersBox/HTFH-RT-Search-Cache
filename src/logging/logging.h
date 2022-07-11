#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_LOGGING_H
#define HTFH_RT_SEARCH_CACHE_LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>

#include "../types/typecheck.h"

enum LogLevel {
    LL_ERROR = 0,
    LL_WARN = 1,
    LL_INFO = 2,
    LL_DEBUG = 3,
    LL_TRACE = 4
};

#ifndef ENABLE_LOGGING
#define LOG(level, stream, msg, ...) ({})
#else

void beforeMain() __attribute__((constructor));
void afterMain() __attribute__((destructor));

extern __thread FILE* logFileHandle;

#ifndef __LOG_FILE_HANDLE__
#define LOGS_DIR(path) \
    void beforeMain() {  \
        if (typename(path) != T_POINTER_TO_CHAR) { \
            fprintf(STDERR, "Expected string file path"); \
            exit(1); \
        } \
        time_t rawtime; time(&rawtime); struct tm* timeinfo = localtime(&rawtime); \
        char filepath[1024]; \
        sprintf( \
            filepath, \
            "%s/log_%d-%d-%d_%d-%d-%d.log", \
            path, \
            timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, \
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec \
        ); \
        logFileHandle = fopen(filepath, "w+"); \
        if (logFileHandle == NULL) { \
            fprintf(STDERR, "Unable to open log file %s: ", filepath); \
            perror(logFileHandle); \
            exit(1); \
        }               \
        printf("Opened log file %s\n", filepath); \
    } \
    void afterMain() { \
        if (logFileHandle == NULL) { \
            fprintf(STDERR, "Log file was prematurely closed\n"); \
            exit(1); \
        } \
        fclose(logFileHandle); \
    }
#define __LOG_FILE_HANDLE__ logFileHandle
#endif // __LOG_FILE_HANDLE__

#ifndef MIN_LOG_LEVEL
#define MIN_LOG_LEVEL LL_TRACE
#endif

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
    if (level <= MIN_LOG_LEVEL) { \
        __DEFINE_DATETIME_STRUCTS;  \
        char logEntry[4096]; \
        sprintf( \
            logEntry, \
            __DATETIME_PREFIX "%s(%s:%d) [%s] :: " \
            msg "\n", \
            __GET_DATETIME_FORMAT_VALUES \
            __func__, __FILENAME__, __LINE__, \
            logLevelToString(level), \
            ##__VA_ARGS__ \
        ); \
        fprintf(level == LL_ERROR ? STDERR : STDOUT, logEntry); \
        fflush(level == LL_ERROR ? STDERR : STDOUT); \
        if (__LOG_FILE_HANDLE__ != NULL) {\
            fprintf( __LOG_FILE_HANDLE__, logEntry); \
            fflush(__LOG_FILE_HANDLE__); \
        }\
    } \
}

//#define LOG(level, msg, ...) printf(msg "\n", ##__VA_ARGS__)

#if MIN_LOG_LEVEL <= 0
#define ERROR(msg, ...) (LOG(LL_ERROR, msg, ##__VA_ARGS__))
#else
#define ERROR(msg, ...) ({})
#endif

#if MIN_LOG_LEVEL <= 1
#define WARN(msg, ...) (LOG(LL_WARN, msg, ##__VA_ARGS__))
#else
#define WARN(msg, ...) ({})
#endif

#if MIN_LOG_LEVEL <= 2
#define INFO(msg, ...) (LOG(LL_INFO, msg, ##__VA_ARGS__))
#else
#define INFO(msg, ...) ({})
#endif

#if MIN_LOG_LEVEL <= 3
#define DEBUG(msg, ...) (LOG(LL_DEBUG, msg, ##__VA_ARGS__))
#else
#define DEBUG(msg, ...) ({})
#endif

#if MIN_LOG_LEVEL <= 4
#define TRACE(msg, ...) (LOG(LL_TRACE, msg, ##__VA_ARGS__))
#else
#define TRACE(msg, ...) ({})
#endif

#endif // ENABLE_LOGGING

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_LOGGING_H
