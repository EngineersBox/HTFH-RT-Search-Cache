#pragma once

#ifndef C_HTFH_RT_SEARCH_CACHE
#define C_HTFH_RT_SEARCH_CACHE

#ifdef __cplusplus
extern "C" {
#endif

#include "../allocator/htfh/htfh.h"
#include "dlirs/dlirs.h"
#include "../allocator/thread/lock.h"
#include "../allocator/alloc_manager.h"
#include "cache_backing.h"

#ifdef HTFH_ALLOCATOR
#define LOCALISE_ALLOCATOR_ARG Allocator* allocator = cache->alloc;
#else
#define LOCALISE_ALLOCATOR_ARG
#endif

typedef struct Cache {
    htfh_lock_t rwlock;
    cache_backing_t backing;
    CacheBackingHandlers handlers;
    Allocator* alloc;
} Cache;

Cache* cache_create(size_t heap_size, size_t ht_size, size_t cache_size, CacheBackingHandlers handlers, void* options);
int cache_destroy(Cache* cache);

bool cache_contains(Cache* cache, const char* key);
bool cache_is_full(Cache* cache);

int cache_request(Cache* cache, const char* key, void* value, void** evicted);
void* cache_get(Cache* cache, const char* key);

#ifdef __cplusplus
};
#endif

#endif // C_HTFH_RT_SEARCH_CACHE