#include "cache.h"

#include <stddef.h>
#include <sys/mman.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

Cache* cache_new(size_t heap_size, size_t ht_size, size_t cache_size, size_t window_size, float hirs_ratio) {
    Cache* cache = malloc(sizeof(*cache));
    if (cache == NULL) {
        return NULL;
    }
    int lock_result;
    if ((lock_result = __htfh_rwlock_init(&cache->rwlock, PTHREAD_PROCESS_PRIVATE)) != 0) {
        set_alloc_errno_msg(RWLOCK_LOCK_INIT, strerror(lock_result));
        return NULL;
    }
    cache->alloc = htfh_create(heap_size);
    if (cache->alloc == NULL) {
        return NULL;
    }
    cache->dlirs = dlirs_create(ht_size, cache_size, window_size, hirs_ratio);
    if (cache->dlirs == NULL) {
        return NULL;
    }
    return cache;
}

int cache_destroy(Cache* cache) {
    if (cache == NULL) {
        return 0;
    } else if (__htfh_rwlock_wrlock_handled(&cache->rwlock) != 0) {
        return -1;
    } else if (htfh_destroy(cache->alloc) != 0) {
        return -1;
    } else if (__htfh_rwlock_unlock_handled(&cache->rwlock) != 0) {
        return -1;
    }
    dlirs_destroy(cache->dlirs);
    free(cache);
    return 0;
}