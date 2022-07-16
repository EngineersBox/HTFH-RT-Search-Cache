#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_CACHE_BACKING_H
#define HTFH_RT_SEARCH_CACHE_CACHE_BACKING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "../allocator/alloc_manager.h"

#ifndef cache_backing_t
#define cache_backing_t void*
#warning No backing type set, defaulting to void*
#endif

typedef void* (*CacheBackingCreate)(AM_ALLOCATOR_PARAM size_t ht_size, size_t cache_size, void* opts);
typedef int (*CacheBackingDestroy)(AM_ALLOCATOR_PARAM cache_backing_t cache);
typedef int (*CacheBackingRequest)(AM_ALLOCATOR_PARAM cache_backing_t cache, const char* key, void* value, void** evicted);
typedef bool (*CacheBackingContains)(cache_backing_t cache, const char* key);
typedef bool (*CacheBackingIsFull)(cache_backing_t cache);

typedef struct CacheBackingHandlers {
    CacheBackingCreate createHandler;
    CacheBackingDestroy destroyHandler;
    CacheBackingRequest requestHandler;
    CacheBackingContains containsHandler;
    CacheBackingIsFull isFullHandler;
} CacheBackingHandlers;

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_CACHE_BACKING_H
