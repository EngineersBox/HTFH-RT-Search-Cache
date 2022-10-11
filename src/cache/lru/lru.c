#include "lru.h"

#include <string.h>
#include "../../preprocessor/lambda.h"

LRUCache* lru_create(AM_ALLOCATOR_PARAM size_t ht_size, size_t cache_size, LRUCacheOptions* options) {
    LRUCache* cache = (LRUCache*) am_malloc(sizeof(*cache));
    if (cache == NULL) {
        return NULL;
    }
    cache->dqht = dqht_create(AM_ALLOCATOR_ARG ht_size, options == NULL ? NULL : options->comparator);
    if (cache->dqht == NULL) {
        return NULL;
    }
    cache->cache_size = cache_size;
    return cache;
}

void destroy_dqht(AM_ALLOCATOR_PARAM void* entry, void* _ignored) {
    if (entry == NULL) {
        return;
    }
    am_free(entry);
}

void lru_destroy(AM_ALLOCATOR_PARAM LRUCache* cache) {
    if (cache == NULL || cache->dqht == NULL) {
        return;
    }
    dqht_destroy_handled(
        AM_ALLOCATOR_ARG
        cache->dqht,
        (EntryValueDestroyHandler) destroy_dqht,
        NULL
    );
    am_free(cache);
}

bool lru_contains(LRUCache* cache, const char* key) {
    if (cache == NULL || cache->dqht == NULL || key == NULL) {
        return false;
    }
    return dqht_get(cache->dqht, key) != NULL;
}

bool lru_is_full(LRUCache* cache) {
    if (cache == NULL || cache->dqht == NULL) {
        return false;
    }
    return cache->dqht->ht->count >= cache->cache_size;
}

// -1 = failure, 0 = miss, 1 = hit
int lru_request(AM_ALLOCATOR_PARAM LRUCache* cache, const char* key, void* value, void** evicted) {
    if (cache == NULL || cache->dqht == NULL || key == NULL) {
        return -1;
    }
    *evicted = NULL;
    int ret_val = 0;
    if (cache->dqht->ht->count >= cache->cache_size) {
        if ((*evicted = dqht_pop_front(AM_ALLOCATOR_ARG cache->dqht)) == NULL) {
            return -1;
        }
        ret_val = 1;
    }
    return dqht_insert(AM_ALLOCATOR_ARG cache->dqht, key, value) == 0 ? ret_val : -1;
}

void* lru_get(AM_ALLOCATOR_PARAM LRUCache* cache, const char* key, void** _ignored) {
    if (cache == NULL || cache->dqht == NULL || key == NULL) {
        return NULL;
    }
    void* entry = dqht_remove(AM_ALLOCATOR_ARG cache->dqht, key);
    if (entry == NULL) {
        return NULL;
    }
    return dqht_insert(AM_ALLOCATOR_ARG cache->dqht, key, entry) == 0 ? entry : NULL;
}

int lru_evict(AM_ALLOCATOR_PARAM LRUCache* cache, void** evicted) {
    if (cache == NULL || cache->dqht == NULL) {
        return -1;
    }
    *evicted = NULL;
    return (*evicted = dqht_pop_front(AM_ALLOCATOR_ARG cache->dqht)) != NULL ? 0 : -1;
}