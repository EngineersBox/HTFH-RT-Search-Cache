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
#if ALLOCATOR_TYPE > 0
#if ALLOCATOR_TYPE == 1
    Allocator* allocator = htfh_create(heap_size);
#elif ALLOCATOR_TYPE == 2
    GlibcAllocator* allocator = gca_create(heap_size);
#endif
    if (allocator == NULL) {
        return NULL;
    }
    cache->alloc = allocator;
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
#if ALLOCATOR_TYPE == 1
    if (htfh_destroy(cache->alloc) != 0) {
        return -1;
    }
#elif ALLOCATOR_TYPE == 2
    if (gca_destroy(cache->alloc) != 0) {
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
    if (cache == NULL || htfh_rwlock_rdlock_handled(&cache->rwlock) != 0) {
        return false;
    }
    bool result = cache->handlers.isFullHandler(cache->backing);
    return htfh_rwlock_unlock_handled(&cache->rwlock) != 0 ? false : result;
}

void* cache_get(Cache* cache, const char* key) {
    if (cache == NULL || htfh_rwlock_rdlock_handled(&cache->rwlock) != 0) {
        return NULL;
    }
    void* result = cache->handlers.getHandler(cache->backing, key);
    return htfh_rwlock_unlock_handled(&cache->rwlock) != 0 ? NULL : result;
}

int cache_request(Cache* cache, const char* key, void* value, void** evicted) {
    if (cache == NULL || htfh_rwlock_wrlock_handled(&cache->rwlock) != 0) {
        return -1;
    }
    LOCALISE_ALLOCATOR_ARG
    int result = cache->handlers.requestHandler(AM_ALLOCATOR_ARG cache->backing, key, value, evicted);
    return htfh_rwlock_unlock_handled(&cache->rwlock) != 0 ? -1 : result;
}

int cache_query(Cache* cache, const char* key, void** hitEntry, void** evicted) {
    if (cache == NULL || htfh_rwlock_wrlock_handled(&cache->rwlock) != 0) {
        return -1;
    }
    LOCALISE_ALLOCATOR_ARG
    void* result = cache->handlers.queryHandler(cache->backing, key, hitEntry, evicted);
    return htfh_rwlock_unlock_handled(&cache->rwlock) != 0 ? -1 : result;
}