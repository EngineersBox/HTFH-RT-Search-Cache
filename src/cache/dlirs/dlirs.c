#include "dlirs.h"

#include <stddef.h>
#include <sys/mman.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "../../math_utils.h"

void* dlirs_new(size_t heap_size, size_t ht_size, size_t cache_size, size_t window_size, float hirs_ratio) {
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
    cache->cache_size = cache_size;
    cache->window_size = window_size;

    cache->hirs_ratio = 0.01f;
    cache->hirs_limit = math_max(1.0f, (int)((cache_size * cache->hirs_ratio) + 0.5f));
    cache->lirs_limit = cache_size - cache->hirs_limit;

    cache->hirs_count = 0;
    cache->lirs_count = 0;
    cache->demoted = 0;
    cache->non_resident = 0;

    cache->lirs = dqht_create(ht_size);
    if (cache->lirs == NULL) {
        return NULL;
    }
    cache->hirs = dqht_create(ht_size);
    if (cache->hirs == NULL) {
        return NULL;
    }
    cache->q = dqht_create(ht_size);
    if (cache->q == NULL) {
        return NULL;
    }

    cache->time = 0;

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