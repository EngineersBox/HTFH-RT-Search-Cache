#include "lru.h"

#include <string.h>

LRUCache* lru_create(AM_ALLOCATOR_PARAM size_t ht_size, size_t cache_size, LRUCacheOptions* options) {
    LRUCache* cache = malloc(sizeof(*cache));
    if (cache == NULL) {
        return NULL;
    }
    cache->dqht = dqht_create(AM_ALLOCATOR_ARG ht_size, options == NULL ? strcmp : options->comparator);
    if (cache->dqht == NULL) {
        return NULL;
    }
    cache->cache_size = cache_size;
    return cache;
}

void lru_destroy(AM_ALLOCATOR_PARAM LRUCache* cache) {
    if (cache == NULL) {
        return;
    } else if (cache->dqht != NULL) {
        dqht_destroy(AM_ALLOCATOR_ARG cache->dqht);
    }
    free(cache);
}

bool lru_contains(AM_ALLOCATOR_PARAM LRUCache* cache, const char* key) {
    return lru_get(AM_ALLOCATOR_ARG cache, key) != NULL;
}

bool lru_is_full(LRUCache* cache) {
    if (cache == NULL || cache->dqht == NULL) {
        return false;
    }
    return cache->dqht->ht->count == cache->cache_size;
}

int lru_request(AM_ALLOCATOR_PARAM LRUCache* cache, const char* key, void* value, void** evicted) {
    if (cache == NULL || cache->dqht == NULL || key == NULL) {
        return -1;
    }
    *evicted = NULL;
    if (cache->dqht->ht->count == cache->cache_size) {
        if (lru_evict(AM_ALLOCATOR_ARG cache, evicted) != 0) {
            return -1;
        }
    }
    return dqht_insert(AM_ALLOCATOR_ARG cache->dqht, key, value, evicted);
}

// -1 = failure, 0 = miss, 1 = hit
int lru_query(AM_ALLOCATOR_PARAM LRUCache* cache, const char* key, void** hitEntry, void** evicted) {
    if (cache == NULL || cache->dqht == NULL || key == NULL) {
        return -1;
    }
    *hitEntry = dqht_remove(AM_ALLOCATOR_ARG cache->dqht, key);
    if (*hitEntry == NULL) {
        return 0;
    }
    return dqht_insert(AM_ALLOCATOR_ARG cache->dqht, key, *hitEntry, evicted) == 0 ? 1 : -1;
}

void* lru_get(LRUCache* cache, const char* key) {
    if (cache == NULL || cache->dqht == NULL || key == NULL) {
        return NULL;
    }
    return dqht_get(cache->dqht, key);
}

int lru_evict(AM_ALLOCATOR_PARAM LRUCache* cache, void** evicted) {
    if (cache == NULL || cache->dqht == NULL) {
        return -1;
    }
    *evicted = NULL;
    return (*evicted = dqht_pop_front(AM_ALLOCATOR_ARG cache->dqht)) != NULL ? 0 : -1;
}