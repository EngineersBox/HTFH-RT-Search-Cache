#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_RANDOM_H
#define HTFH_RT_SEARCH_CACHE_RANDOM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdbool.h>

#include "../hashtable/cache_hashtable.h"

typedef struct RandomCache {
    size_t cache_size;
    HashTable* ht;
} RandomCache;

typedef struct RandomCacheOptions {
    KeyComparator comparator;
} RandomCacheOptions;

RandomCache* rc_create(AM_ALLOCATOR_PARAM size_t ht_size, size_t cache_size, RandomCacheOptions* options);
void rc_destroy(AM_ALLOCATOR_PARAM RandomCache* cache);

bool rc_contains(AM_ALLOCATOR_PARAM RandomCache* cache, const char* key);
bool rc_is_full(RandomCache* cache);

int rc_request(AM_ALLOCATOR_PARAM RandomCache* cache, const char* key, void* value, void** evicted);
// -1 = failure, 0 = miss, 1 = hit
int rc_query(AM_ALLOCATOR_PARAM RandomCache* cache, const char* key, void** hitEntry, void** evicted);
void* rc_get(RandomCache* cache, const char* key);

void* rc_evict_random(AM_ALLOCATOR_PARAM RandomCache* cache);
void* rc_evict_by_key(AM_ALLOCATOR_PARAM RandomCache* cache, const char* key);

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_RANDOM_H
