#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_RANDOM_H
#define HTFH_RT_SEARCH_CACHE_RANDOM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>

#include "../hashtable/cache_hashtable.h"

typedef struct RandomCache {
    size_t cache_size;
    HashTable* ht;
} RandomCache;

RandomCache* rc_create(size_t ht_size, size_t cache_size);
void rc_destroy(RandomCache* cache);

// -1 = failure, 0 = success, 1 = evicted + added
int rc_add(RandomCache* cache, const char* key, void* value, void* evicted);

void* rc_evict_random(RandomCache* cache);

void* rc_evict_by_key(RandomCache* cache, const char* key);

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_RANDOM_H
