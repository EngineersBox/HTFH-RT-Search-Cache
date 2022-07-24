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
#include <string.h>

#include "../types/typecheck.h"

enum LogLevel {
    LL_FATAL = 0,
    LL_ERROR = 1,
    LL_WARN = 2,
    LL_INFO = 3,
    LL_DEBUG = 4,
    LL_TRACE = 5
};

#ifndef ENABLE_LOGGING
#define LOG(level, msg, ...)
#define LOGS_DIR(path)
#else

#ifndef MIN_LOG_LEVEL
#define MIN_LOG_LEVEL LL_TRACE
#endif

void beforeMain() __attribute__((constructor));
void afterMain() __attribute__((destructor));

extern _Thread_local char logFileName[1024];
extern _Thread_local FILE* logFileHandle;

// Log streams
#define STDOUT stdout
#define STDERR stderr

#ifndef LOG_FILE_HANDLE
#define LOGS_DIR(path) \
    void beforeMain() { \
        if (typename(path) != T_POINTER_TO_CHAR) { \
            fprintf(STDERR, "[LOGGER] Expected string file path"); \
            exit(1); \
        } \
        time_t rawtime; time(&rawtime); struct tm* timeinfo = localtime(&rawtime); \
        char filepath[1024]; \
        sprintf( \
            filepath, \
            "%s/log_%d-%d-%d_%d-%d-%d.log", \
            path, \
            timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, \
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec \
        ); \
        strcpy(logFileName, filepath); \
        logFileHandle = fopen(filepath, "w+"); \
        if (logFileHandle == NULL) { \
            fprintf(STDERR, "[LOGGER] Unable to open log file %s: ", filepath); \
            perror(logFileHandle); \
            exit(1); \
        }               \
        printf("[LOGGER] Opened log file %s\n", filepath); \
    } \
    void afterMain() { \
        if (logFileHandle == NULL) { \
            fprintf(STDERR, "[LOGGER] Log file was prematurely closed\n"); \
            exit(1); \
        } \
        fclose(logFileHandle); \
        printf("[LOGGER] Closed log file %s\n", logFileName); \
    }
#define LOG_FILE_HANDLE logFileHandle
#endif // LOG_FILE_HANDLE

static inline char* logLevelToString(int level) {
    if (level == LL_FATAL) {
        return "FATAL";
    } else if (level == LL_ERROR)  {
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

#define FILENAME (strrchr("/" __FILE__, '/') + 1)

#ifdef LOG_DATETIME_PREFIX
#define GET_DATETIME_FORMAT_VALUES timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
#define DATETIME_PREFIX "[%d/%d/%d %d:%d:%d] "
#define DEFINE_DATETIME_STRUCTS time_t rawtime; time(&rawtime); struct tm* timeinfo = localtime(&rawtime);
#else
#define __GET_DATETIME_FORMAT_VALUES
#define __DATETIME_PREFIX ""
#define __DEFINE_DATETIME_STRUCTS ({});
#endif

//#define LOG(level, msg, ...) { \
//    if (typename(level) != T_INT) { \
//        fprintf(STDERR, "[LOGGER] Expected integer log level"); \
//        exit(1); \
//    } \
//    if (level <= MIN_LOG_LEVEL) { \
//        DEFINE_DATETIME_STRUCTS;  \
//        char logEntry[4096]; \
//        sprintf( \
//            logEntry, \
//            DATETIME_PREFIX "%s(%s:%d) [%s] :: " \
//            msg "\n", \
//            GET_DATETIME_FORMAT_VALUES \
//            __func__, FILENAME, __LINE__, \
//            logLevelToString(level), \
//            ##__VA_ARGS__ \
//        ); \
//        fprintf(level == LL_ERROR ? STDERR : STDOUT, logEntry); \
//        fflush(level == LL_ERROR ? STDERR : STDOUT); \
//        if (LOG_FILE_HANDLE != NULL) {\
//            fprintf( LOG_FILE_HANDLE, logEntry); \
//            fflush(LOG_FILE_HANDLE); \
//        }\
//    } \
//}
//#define LOG(level, msg, ...) { \
//    if (typename(level) != T_INT) { \
//        fprintf(STDERR, "[LOGGER] Expected integer log level"); \
//        exit(1); \
//    } \
//    if (level <= MIN_LOG_LEVEL) { \
//        DEFINE_DATETIME_STRUCTS;  \
//        fprintf( \
//            level == LL_ERROR ? STDERR : STDOUT, \
//            DATETIME_PREFIX "%s(%s:%d) [%s] :: " \
//            msg "\n", \
//            GET_DATETIME_FORMAT_VALUES \
//            __func__, FILENAME, __LINE__, \
//            logLevelToString(level), \
//            ##__VA_ARGS__ \
//        ); \
//        fflush(level == LL_ERROR ? STDERR : STDOUT); \
//    } \
//}
#define LOG(level, msg, ...) printf(msg "\n", ##__VA_ARGS__)

#endif // ENABLE_LOGGING

#if MIN_LOG_LEVEL <= 0
#define FATAL(msg, ...) LOG(LL_FATAL, msg, ##__VA_ARGS__); exit(1)
#else
#define FATAL(msg, ...)
#endif

#if MIN_LOG_LEVEL <= 1
#define ERROR(msg, ...) LOG(LL_ERROR, msg, ##__VA_ARGS__)
#else
#define ERROR(msg, ...)
#endif

#if MIN_LOG_LEVEL <= 2
#define WARN(msg, ...) LOG(LL_WARN, msg, ##__VA_ARGS__)
#else
#define WARN(msg, ...)
#endif

#if MIN_LOG_LEVEL <= 3
#define INFO(msg, ...) LOG(LL_INFO, msg, ##__VA_ARGS__)
#else
#define INFO(msg, ...)
#endif

#if MIN_LOG_LEVEL <= 4
#define DEBUG(msg, ...) LOG(LL_DEBUG, msg, ##__VA_ARGS__)
#else
#define DEBUG(msg, ...)
#endif

#if MIN_LOG_LEVEL <= 5
#define TRACE(msg, ...) LOG(LL_TRACE, msg, ##__VA_ARGS__)
#else
#define TRACE(msg, ...)
#endif

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_LOGGING_H
