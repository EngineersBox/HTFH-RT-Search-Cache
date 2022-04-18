#include "cache.h"

#include <stddef.h>
#include <sys/mman.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

void* cache_new(size_t heap_size) {
    Cache* cache = malloc(sizeof(*cache));
    if (cache == NULL) {
        return NULL;
    }
    Allocator* alloc = htfh_create(heap_size);
    if (cache->alloc == NULL) {
        return NULL;
    }
    int lock_result;
    if ((lock_result = __htfh_lock_init(&alloc->mutex, PTHREAD_MUTEX_RECURSIVE)) != 0) {
        set_alloc_errno_msg(MUTEX_LOCK_INIT, strerror(lock_result));
        return NULL;
    }
    return alloc;
}

int cache_free(Cache* cache) {
    if (cache == NULL) {
        return 0;
    } else if (__htfh_lock_lock_handled(&cache->mutex) != 0) {
        return -1;
    } else if (htfh_destroy(cache->alloc) != 0) {
        return -1;
    } else if (__htfh_lock_unlock_handled(&cache->mutex) != 0) {
        return -1;
    }
    free(cache);
    return 0;
}