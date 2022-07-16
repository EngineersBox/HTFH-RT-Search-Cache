#include "lru.h"

LRUCache* lru_create(AM_ALLOCATOR_PARAM size_t ht_size, size_t cache_size) {
    LRUCache* cache = malloc(sizeof(*cache));
    if (cache == NULL) {
        return NULL;
    }
    cache->dqht = dqht_create(AM_ALLOCATOR_ARG ht_size);
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

// -1 = failure, 0 = success, 1 = evicted + added
int lru_add(AM_ALLOCATOR_PARAM LRUCache* cache, const char* key, void* value, void** evicted) {
    if (cache == NULL || cache->dqht == NULL || key == NULL) {
        return -1;
    }
    *evicted = NULL;
    int ret_val = 0;
    if (cache->dqht->ht->count == cache->cache_size) {
        if (lru_evict(AM_ALLOCATOR_ARG cache, evicted) != 0) {
            return -1;
        }
        ret_val = 1;
    }
    return dqht_insert(AM_ALLOCATOR_ARG cache->dqht, key, value) == 0 ? ret_val : -1;
}

int lru_evict(AM_ALLOCATOR_PARAM LRUCache* cache, void** evicted) {
    if (cache == NULL || cache->dqht == NULL) {
        return -1;
    }
    *evicted = NULL;
    return (*evicted = dqht_pop_front(AM_ALLOCATOR_ARG cache->dqht)) != NULL ? 0 : -1;
}