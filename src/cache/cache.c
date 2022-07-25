#include "cache.h"

#include <stddef.h>
#include <sys/mman.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "../logging/logging.h"

Cache* cache_create(size_t heap_size, size_t ht_size, size_t cache_size, CacheBackingHandlers handlers, void* options) {
    Cache* cache = malloc(sizeof(*cache));
    if (cache == NULL) {
        return NULL;
    }
    cache->handlers = handlers;
    init_check(int, lock_result, htfh_rwlock_init(&cache->rwlock, PTHREAD_PROCESS_PRIVATE), != 0) {
        set_alloc_errno_msg(RWLOCK_LOCK_INIT, strerror(lock_result));
        return NULL;
    }
#ifdef HTFH_ALLOCATOR
    DEBUG("Using HTFH allocator for cache entries");
    Allocator* allocator = htfh_create(heap_size);
    if (allocator == NULL) {
        return NULL;
    }
    cache->alloc = allocator;
#else
    DEBUG("Using core allocator for cache entries");
#endif
    cache->backing = handlers.createHandler(AM_ALLOCATOR_ARG ht_size, cache_size, options);
    if (cache->backing == NULL) {
        return NULL;
    }
    return cache;
}

int cache_destroy(Cache* cache) {
    if (cache == NULL) {
        return 0;
    } else if (htfh_rwlock_wrlock_handled(&cache->rwlock) != 0) {
        return -1;
    }
    LOCALISE_ALLOCATOR_ARG
    if (cache->handlers.destroyHandler(AM_ALLOCATOR_ARG cache->backing) != 0) {
        return -1;
    }
#ifdef HTFH_ALLOCATOR
    if (htfh_destroy(cache->alloc) != 0) {
        return -1;
    }
#endif
    if (htfh_rwlock_unlock_handled(&cache->rwlock) != 0) {
        return -1;
    } else if (htfh_rwlock_destroy_handled(&cache->rwlock) != 0) {
        return -1;
    }
    free(cache);
    return 0;
}

bool cache_contains(Cache* cache, const char* key) {
    if (cache == NULL || htfh_rwlock_rdlock_handled(&cache->rwlock) != 0) {
        return false;
    }
    bool result = cache->handlers.containsHandler(cache->backing, key);
    return htfh_rwlock_unlock_handled(&cache->rwlock) != 0 ? false : result;
}

bool cache_is_full(Cache* cache) {
    if (cache == NULL) {
        return false;
    }
    return cache->handlers.isFullHandler(cache->backing);
}

int cache_request(Cache* cache, const char* key, void* value, void** evicted) {
    if (cache == NULL || htfh_rwlock_wrlock_handled(&cache->rwlock) != 0) {
        return -1;
    }
    LOCALISE_ALLOCATOR_ARG
    int result = cache->handlers.requestHandler(AM_ALLOCATOR_ARG cache->backing, key, value, evicted);
    int lockResult = htfh_rwlock_unlock_handled(&cache->rwlock);
    printf("Lock result: %d, Request result %d\n", lockResult, result);
    return lockResult != 0 ? lockResult : result;
}

void* cache_get(Cache* cache, const char* key) {
    if (cache == NULL || htfh_rwlock_rdlock_handled(&cache->rwlock) != 0) {
        return NULL;
    }
    void* result = cache->handlers.getHandler(cache->backing, key);
    return htfh_rwlock_unlock_handled(&cache->rwlock) != 0 ? NULL : result;
}