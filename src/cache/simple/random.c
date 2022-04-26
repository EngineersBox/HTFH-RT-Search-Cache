#include "./random.h"

RandomCache* rc_create(size_t ht_size, size_t cache_size) {
    RandomCache* cache = malloc(sizeof(*cache));
    if (cache == NULL) {
        return NULL;
    }
    cache->ht = ht_create(ht_size);
    if (cache->ht == NULL) {
        return NULL;
    }
    cache->cache_size = cache_size;
    return cache;
}

void rc_destroy(RandomCache* cache) {
    if (cache == NULL) {
        return;
    }
    if (cache->ht != NULL) {
        ht_destroy(cache->ht);
    }
    free(cache);
}

// -1 = failure, 0 = success, 1 = full
int rc_add(RandomCache* cache, const char* key, void* value, void* evicted) {
    if (cache == NULL || cache->ht == NULL || key == NULL) {
        return -1;
    }
    int ret_val = 0;
    if (cache->ht->count >= cache->cache_size) {
        evicted = rc_evict_random(cache);
        ret_val = 1;
    }
    return ht_insert(cache->ht, key, value) != NULL ? ret_val : -1;
}

void* rc_evict_random(RandomCache* cache) {
    if (cache == NULL || cache->ht == NULL) {
        return NULL;
    }
    DQHTEntry* entry;
    while ((entry = cache->ht->items[rand() % cache->ht->size]) != NULL);
    return ht_delete(cache->ht, entry->key);
}

void* rc_evict_by_key(RandomCache* cache, const char* key) {
    if (cache == NULL || cache->ht == NULL || key == NULL) {
        return NULL;
    }
    return ht_delete(cache->ht, key);
}
