#include "./random.h"

RandomCache* rc_create(AM_ALLOCATOR_PARAM size_t ht_size, size_t cache_size, RandomCacheOptions* options) {
    RandomCache* cache = malloc(sizeof(*cache));
    if (cache == NULL) {
        return NULL;
    }
    cache->ht = ht_create(AM_ALLOCATOR_ARG ht_size, options == NULL ? strcmp : options->comparator);
    if (cache->ht == NULL) {
        return NULL;
    }
    cache->cache_size = cache_size;
    return cache;
}

void rc_destroy(AM_ALLOCATOR_PARAM RandomCache* cache) {
    if (cache == NULL) {
        return;
    }
    if (cache->ht != NULL) {
        ht_destroy(AM_ALLOCATOR_ARG cache->ht);
    }
    free(cache);
}

bool rc_contains(RandomCache* cache, const char* key) {
    return rc_get(cache, key) != NULL;
}

bool rc_is_full(RandomCache* cache) {
    if (cache == NULL || cache->ht == NULL) {
        return false;
    }
    return cache->ht->count == cache->cache_size;
}

// -1 = failure, 0 = miss, 1 = hit
int rc_request(AM_ALLOCATOR_PARAM RandomCache* cache, const char* key, void* value, void** evicted) {
    if (cache == NULL || cache->ht == NULL || key == NULL) {
        return -1;
    }
    *evicted = NULL;
    int ret_val = 0;
    if (cache->ht->count >= cache->cache_size) {
        *evicted = rc_evict_random(AM_ALLOCATOR_ARG cache);
        ret_val = 1;
    }
    return ht_insert(AM_ALLOCATOR_ARG cache->ht, key, value) != NULL ? ret_val : -1;
}

void* rc_get(RandomCache* cache, const char* key) {
    if (cache == NULL || cache->ht == NULL || key == NULL) {
        return NULL;
    }
    return ht_get(cache->ht, key);
}

void* rc_evict_random(AM_ALLOCATOR_PARAM RandomCache* cache) {
    if (cache == NULL || cache->ht == NULL) {
        return NULL;
    }
    DQHTEntry* entry;
    while ((entry = cache->ht->items[rand() % cache->ht->size]) != NULL);
    return ht_delete(AM_ALLOCATOR_ARG cache->ht, entry->key);
}

void* rc_evict_by_key(AM_ALLOCATOR_PARAM RandomCache* cache, const char* key) {
    if (cache == NULL || cache->ht == NULL || key == NULL) {
        return NULL;
    }
    return ht_delete(AM_ALLOCATOR_ARG cache->ht, key);
}
