#include "lru.h"

LRUCache* lru_create(size_t ht_size, size_t cache_size) {
    LRUCache* cache = malloc(sizeof(*cache));
    if (cache == NULL) {
        return NULL;
    }
    cache->dqht = dqht_create(ht_size);
    if (cache->dqht == NULL) {
        return NULL;
    }
    cache->cache_size = cache_size;
    return cache;
}

void lru_destroy(LRUCache* cache) {
    if (cache == NULL) {
        return;
    } else if (cache->dqht != NULL) {
        dqht_destroy(cache->dqht);
    }
    free(cache);
}

// -1 = failure, 0 = success, 1 = evicted + added
int lru_add(LRUCache* cache, const char* key, void* value, void* evicted) {
    if (cache == NULL || cache->dqht == NULL || key == NULL) {
        return -1;
    }
    int ret_val = 0;
    if (cache->dqht->ht->count == cache->cache_size) {
        if (lru_evict(cache, evicted) != 0) {
            return -1;
        }
        ret_val = 1;
    }
    return dqht_insert(cache->dqht, key, value) == 0 ? ret_val : -1;
}

int lru_evict(LRUCache* cache, void* evicted) {
    if (cache == NULL || cache->dqht == NULL) {
        return -1;
    }
    return (evicted = dqht_pop_front(cache->dqht)) != NULL ? 0 : -1;
}