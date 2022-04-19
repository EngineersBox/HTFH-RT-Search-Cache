#include "dlirs.h"

#include <stddef.h>
#include <sys/mman.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

void* dlirs_new(size_t heap_size) {
    DLIRS* cache = malloc(sizeof(*cache));
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
    return cache;
}

int dlirs_destroy(DLIRS* cache) {
    if (cache == NULL) {
        return 0;
    } else if (__htfh_rwlock_wrlock_handled(&cache->rwlock) != 0) {
        return -1;
    } else if (htfh_destroy(cache->alloc) != 0) {
        return -1;
    } else if (__htfh_rwlock_unlock_handled(&cache->rwlock) != 0) {
        return -1;
    }
    free(cache);
    return 0;
}