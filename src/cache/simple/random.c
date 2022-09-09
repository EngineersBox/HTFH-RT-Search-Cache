#include "./random.h"

#include <string.h>

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

bool rc_contains(AM_ALLOCATOR_PARAM RandomCache* cache, const char* key) {
    void* entry;
    void* ignoredEvicted;
    return rc_get(AM_ALLOCATOR_ARG cache, key, &entry, &ignoredEvicted) == 0 && entry != NULL;
}

bool rc_is_full(RandomCache* cache) {
    if (cache == NULL || cache->ht == NULL) {
        return false;
    }
    return cache->ht->count == cache->cache_size;
}

int rc_request(AM_ALLOCATOR_PARAM RandomCache* cache, const char* key, void* value, void** evicted) {
    if (cache == NULL || cache->ht == NULL || key == NULL) {
        return -1;
    }
    *evicted = NULL;
    if (cache->ht->count >= cache->cache_size) {
        *evicted = rc_evict_random(AM_ALLOCATOR_ARG cache);
    }
    DQHTEntry* ignoredEntry;
    return ht_insert(AM_ALLOCATOR_ARG cache->ht, key, value, &ignoredEntry) != -1 ? 0 : -1;
}

// -1 = failure, 0 = miss, 1 = hit
int rc_get(AM_ALLOCATOR_PARAM RandomCache* cache, const char* key, void** hitEntry, void** evicted) {
    if (cache == NULL || cache->ht == NULL || key == NULL) {
        return -1;
    }
    return (*hitEntry = ht_get(cache->ht, key)) == NULL;
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
