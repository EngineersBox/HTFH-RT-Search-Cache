#include "cache.h"

#include <stddef.h>
#include <sys/mman.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

Cache* cache_create(size_t heap_size, size_t ht_size, size_t cache_size, float hirs_ratio) {
    Cache* cache = malloc(sizeof(*cache));
    if (cache == NULL) {
        return NULL;
    }
    int lock_result;
    if ((lock_result = __htfh_rwlock_init(&cache->rwlock, PTHREAD_PROCESS_PRIVATE)) != 0) {
        set_alloc_errno_msg(RWLOCK_LOCK_INIT, strerror(lock_result));
        return NULL;
    }
#ifdef HTFH_ALLOCATOR
    Allocator* allocator = htfh_create(heap_size);
    if (allocator == NULL) {
        return NULL;
    }
    cache->alloc = allocator;
#endif
    cache->dlirs = dlirs_create(AM_ALLOCATOR_ARG ht_size, cache_size, hirs_ratio);
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
    }
    LOCALISE_ALLOCATOR_ARG
    dlirs_destroy(AM_ALLOCATOR_ARG cache->dlirs);
    if (htfh_destroy(cache->alloc) != 0) {
        return -1;
    } else if (__htfh_rwlock_unlock_handled(&cache->rwlock) != 0) {
        return -1;
    }
    free(cache);
    return 0;
}

bool cache_contains(Cache* cache, const char* key) {
    if (cache == NULL || __htfh_rwlock_wrlock_handled(&cache->rwlock) != 0) {
        return false;
    }
    bool result = dlirs_contains(cache->dlirs, key);
    return __htfh_rwlock_unlock_handled(&cache->rwlock) != 0 ? false : result;
}

bool cache_is_full(Cache* cache) {
    if (cache == NULL) {
        return false;
    }
    return dlirs_is_full(cache->dlirs);
}

int cache_request(Cache* cache, const char* key, void* value, DLIRSEntry** evicted) {
    if (cache == NULL || __htfh_rwlock_wrlock_handled(&cache->rwlock) != 0) {
        return -1;
    }
    LOCALISE_ALLOCATOR_ARG
    int result = dlirs_request(AM_ALLOCATOR_ARG cache->dlirs, key, value, evicted);
    int lockResult = __htfh_rwlock_unlock_handled(&cache->rwlock);
    return lockResult != 0 ? lockResult : result;
}