#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_CACHE_BACKING_H
#define HTFH_RT_SEARCH_CACHE_CACHE_BACKING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "../allocator/alloc_manager.h"

#ifndef CACHE_BACKING_TYPE
#define CACHE_BACKING_TYPE void*
#warning "No backing type set, defaulting to void*"
#endif

typedef void* (*CacheBackingCreate)(AM_ALLOCATOR_PARAM size_t ht_size, size_t cache_size, void* opts);
typedef int (*CacheBackingDestroy)(AM_ALLOCATOR_PARAM CACHE_BACKING_TYPE cache);
typedef int (*CacheBackingRequest)(AM_ALLOCATOR_PARAM CACHE_BACKING_TYPE cache, const char* key, void* value, void** evicted);
typedef bool (*CacheBackingContains)(CACHE_BACKING_TYPE cache, const char* key);
typedef bool (*CacheBackingIsFull)(CACHE_BACKING_TYPE cache);

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
