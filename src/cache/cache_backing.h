#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_CACHE_BACKING_H
#define HTFH_RT_SEARCH_CACHE_CACHE_BACKING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "../allocator/alloc_manager.h"
#include "dlirs/dlirs.h"
#include "simple/random.h"

#ifndef cache_backing_t
#define cache_backing_t void*
#warning No backing type set, defaulting to void*
#endif

typedef cache_backing_t (*CacheBackingCreate)(AM_ALLOCATOR_PARAM size_t ht_size, size_t cache_size, void* opts);
typedef int (*CacheBackingDestroy)(AM_ALLOCATOR_PARAM cache_backing_t cache);
typedef int (*CacheBackingRequest)(AM_ALLOCATOR_PARAM cache_backing_t cache, const char* key, void* value, void** evicted);
typedef void* (*CacheBackingGet)(cache_backing_t cache, const char* key);
typedef bool (*CacheBackingContains)(cache_backing_t cache, const char* key);
typedef bool (*CacheBackingIsFull)(cache_backing_t cache);

typedef struct CacheBackingHandlers {
    CacheBackingCreate createHandler;
    CacheBackingDestroy destroyHandler;
    CacheBackingRequest requestHandler;
    CacheBackingGet getHandler;
    CacheBackingContains containsHandler;
    CacheBackingIsFull isFullHandler;
} CacheBackingHandlers;

#define DLIRS_CACHE_BACKING_HANDLERS ((CacheBackingHandlers) { \
    .createHandler = (CacheBackingCreate) dlirs_create, \
    .destroyHandler = dlirs_destroy, \
    .containsHandler = dlirs_contains, \
    .isFullHandler = dlirs_is_full, \
    .requestHandler = (CacheBackingRequest) dlirs_request, \
    .getHandler = dlirs_get \
})

#define RANDOM_CACHE_BACKING_HANDLERS ((CacheBackingHandlers) { \
    .createHandler = (CacheBackingCreate) rc_create, \
    .destroyHandler = rc_destroy, \
    .containsHandler = rc_contains, \
    .isFullHandler = rc_is_full, \
    .requestHandler = (CacheBackingRequest) rc_request, \
    .getHandler = rc_get \
})

#define LRU_CACHE_BACKING_HANDLERS ((CacheBackingHandlers) { \
    .createHandler = (CacheBackingCreate) lru_create, \
    .destroyHandler = lru_destroy, \
    .containsHandler = lru_contains, \
    .isFullHandler = lru_is_full, \
    .requestHandler = (CacheBackingRequest) lru_request, \
    .getHandler = lru_get \
})

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_CACHE_BACKING_H
