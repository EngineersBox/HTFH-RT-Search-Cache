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

RandomCache* rc_create(AM_ALLOCATOR_PARAM size_t ht_size, size_t cache_size, void* options);
void rc_destroy(AM_ALLOCATOR_PARAM RandomCache* cache);

bool rc_contains(RandomCache* cache, const char* key);
bool rc_is_full(RandomCache* cache);

// -1 = failure, 0 = success, 1 = evicted + added
int rc_request(AM_ALLOCATOR_PARAM RandomCache* cache, const char* key, void* value, void** evicted);

void* rc_evict_random(AM_ALLOCATOR_PARAM RandomCache* cache);

void* rc_evict_by_key(AM_ALLOCATOR_PARAM RandomCache* cache, const char* key);

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_RANDOM_H
