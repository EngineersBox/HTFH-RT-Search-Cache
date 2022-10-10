#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_LRU_H
#define HTFH_RT_SEARCH_CACHE_LRU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <inttypes.h>
#include <stddef.h>

#include "../dequeue_hashtable/dq_hashtable.h"

typedef struct LRUCache {
    size_t cache_size;
    DequeueHashTable* dqht;
} LRUCache;

typedef struct LRUCacheOptions {
    KeyComparator comparator;
} LRUCacheOptions;

LRUCache* lru_create(AM_ALLOCATOR_PARAM size_t ht_size, size_t cache_size, LRUCacheOptions* options);
void lru_destroy(AM_ALLOCATOR_PARAM LRUCache* cache);

bool lru_contains(LRUCache* cache, const char* key);
bool lru_is_full(LRUCache* cache);

// -1 = failure, 0 = miss, 1 = hit
int lru_request(AM_ALLOCATOR_PARAM LRUCache* cache, const char* key, void* value, void** evicted);
void* lru_get(AM_ALLOCATOR_PARAM LRUCache* cache, const char* key, void** _ignored);

int lru_evict(AM_ALLOCATOR_PARAM LRUCache* cache, void** evicted);

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_LRU_H
