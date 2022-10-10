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
#include "lru/lru.h"

#ifndef cache_backing_t
#define cache_backing_t void*
#warning No backing type set, defaulting to void*
#endif

typedef cache_backing_t (*CacheBackingCreate)(AM_ALLOCATOR_PARAM size_t ht_size, size_t cache_size, void* opts);
typedef int (*CacheBackingDestroy)(AM_ALLOCATOR_PARAM cache_backing_t cache);
typedef int (*CacheBackingRequest)(AM_ALLOCATOR_PARAM cache_backing_t cache, const char* key, void* value, void** evicted);
typedef void* (*CacheBackingGet)(AM_ALLOCATOR_PARAM cache_backing_t cache, const char* key, void** evicted);
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
    .destroyHandler = (CacheBackingDestroy) dlirs_destroy, \
    .requestHandler = (CacheBackingRequest) dlirs_request, \
    .getHandler = (CacheBackingGet) dlirs_get ,\
    .containsHandler = (CacheBackingContains) dlirs_contains, \
    .isFullHandler = (CacheBackingIsFull) dlirs_is_full \
})

#define RANDOM_CACHE_BACKING_HANDLERS ((CacheBackingHandlers) { \
    .createHandler = (CacheBackingCreate) rc_create, \
    .destroyHandler = (CacheBackingDestroy) rc_destroy, \
    .requestHandler = (CacheBackingRequest) rc_request, \
    .getHandler = (CacheBackingGet) rc_get, \
    .containsHandler = (CacheBackingContains) rc_contains, \
    .isFullHandler = (CacheBackingIsFull) rc_is_full \
})

#define LRU_CACHE_BACKING_HANDLERS ((CacheBackingHandlers) { \
    .createHandler = (CacheBackingCreate) lru_create, \
    .destroyHandler = (CacheBackingDestroy) lru_destroy, \
    .requestHandler = (CacheBackingRequest) lru_request, \
    .getHandler = (CacheBackingGet) lru_get, \
    .containsHandler = (CacheBackingContains) lru_contains, \
    .isFullHandler = (CacheBackingIsFull) lru_is_full \
})

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_CACHE_BACKING_H
