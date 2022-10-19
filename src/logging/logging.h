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
#include <stdbool.h>
#include <pthread.h>
#include <inttypes.h>

#include "ansi_colour_codes.h"
#include "../types/typecheck.h"

typedef enum LogLevel {
    LL_FATAL = 0,
    LL_ERROR = 1,
    LL_WARN = 2,
    LL_INFO = 3,
    LL_DEBUG = 4,
    LL_TRACE = 5
} LogLevel;

#ifndef ENABLE_LOGGING
#define LOG(level, msg, ...)
#define LOGS_DIR(path)
#else

#ifndef MIN_LOG_LEVEL
#define MIN_LOG_LEVEL LL_TRACE
#endif

void beforeMain() __attribute__((constructor));
void afterMain() __attribute__((destructor));

extern volatile char logFileName[1024];
extern volatile FILE* logFileHandle;

// Log streams
#define STDOUT stdout
#define STDERR stderr

#ifndef LOG_FILE_HANDLE
#define LOGS_DIR(path) \
    void beforeMain() { \
        if (typename(path) != T_POINTER_TO_CHAR) { \
            fprintf(STDERR, "[" B_MAGENTA "LOGGER" RESET "]" RED " Expected string file path"); \
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
            fprintf(STDERR, "[" B_MAGENTA "LOGGER" RESET "]" RED " Unable to open log file %s: ", filepath); \
            perror(logFileHandle); \
            exit(1); \
        }               \
        printf("[" B_MAGENTA "LOGGER" RESET "] Opened log file %s\n", filepath); \
    } \
    void afterMain() { \
        if (logFileHandle == NULL) { \
            fprintf(STDERR, "[" B_MAGENTA "LOGGER" RESET "]" RED " Log file was prematurely closed\n"); \
            exit(1); \
        } \
        fclose(logFileHandle); \
        printf("[" B_MAGENTA "LOGGER" RESET "] Closed log file %s\n", logFileName); \
    }
#define LOG_FILE_HANDLE logFileHandle
#endif // LOG_FILE_HANDLE

static inline char* logLevelToString(LogLevel level, bool raw) {
#define LL_CONVERT_FORMAT(prefix, str, suffix) (raw ? str : prefix str suffix)
    switch (level) {
        case LL_FATAL:
            return LL_CONVERT_FORMAT(B_RED, "FATAL", RESET);
        case LL_ERROR:
            return LL_CONVERT_FORMAT(RED, "ERROR", RESET);
        case LL_WARN:
            return LL_CONVERT_FORMAT(YELLOW, "WARN ", RESET);
        case LL_DEBUG:
            return LL_CONVERT_FORMAT(MAGENTA, "DEBUG", RESET);
        case LL_TRACE:
            return LL_CONVERT_FORMAT(CYAN, "TRACE", RESET);
        default:
            return LL_CONVERT_FORMAT(GREEN, "INFO ", RESET);
    }
#undef LL_CONVERT_FORMAT
}

#define FILENAME (strrchr("/" __FILE__, '/') + 1)

#ifdef LOG_DATETIME_PREFIX
#define GET_DATETIME_FORMAT_VALUES timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
#define DATETIME_PREFIX "[" MAGENTA "%d/%d/%d %d:%d:%d" RESET "] "
#define RAW_DATETIME_PREFIX "[%d/%d/%d %d:%d:%d] "
#define DEFINE_DATETIME_STRUCTS time_t rawtime; time(&rawtime); struct tm* timeinfo = localtime(&rawtime);
#else
#define GET_DATETIME_FORMAT_VALUES
#define DATETIME_PREFIX ""
#define RAW_DATETIME_PREFIX ""
#define DEFINE_DATETIME_STRUCTS ({});
#endif

#define LOG(level, msg, ...) { \
    if (typename(level) != T_INT) { \
        fprintf(STDERR, "[" B_MAGENTA "LOGGER" RESET "]" B_RED " Expected integer log level"); \
        exit(1); \
    } \
    if (level <= MIN_LOG_LEVEL) { \
        DEFINE_DATETIME_STRUCTS;  \
        char logEntry[4096]; \
        sprintf( \
            logEntry, \
            DATETIME_PREFIX "[" YELLOW "%zu" RESET "] " H_BLUE "%s" RESET "(%s:" MAGENTA "%d" RESET ") [%s] :: " \
            msg "\n", \
            GET_DATETIME_FORMAT_VALUES \
            (uintptr_t) pthread_self(), \
            __func__, FILENAME, __LINE__, \
            logLevelToString(level, false), \
            ##__VA_ARGS__ \
        ); \
        fprintf(level == LL_ERROR ? STDERR : STDOUT, logEntry); \
        fflush(level == LL_ERROR ? STDERR : STDOUT);  \
        if (LOG_FILE_HANDLE != NULL) {\
            char fileEntry[4096]; \
            sprintf( \
                fileEntry, \
                RAW_DATETIME_PREFIX "[%zu] %s(%s:%d) [%s] :: " \
                msg "\n", \
                GET_DATETIME_FORMAT_VALUES \
                (uintptr_t) pthread_self(), \
                __func__, FILENAME, __LINE__, \
                logLevelToString(level, true), \
                ##__VA_ARGS__ \
            ); \
            fprintf(LOG_FILE_HANDLE, fileEntry); \
            fflush(LOG_FILE_HANDLE); \
        }\
    } \
}

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
