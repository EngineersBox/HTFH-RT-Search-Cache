#include "dlirs.h"

#include <stddef.h>
#include <sys/mman.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

int dlirs_contains(DLIRS* cache, const char* key) {
    if (cache == NULL || cache->lirs || cache->q || key == NULL) {
        return -1;
    }
    DLIRSEntry* value;
    if ((value = dqht_get(cache->lirs, key)) != NULL) {
        return value->in_cache ? 0 : -1;
    }
    return dqht_get(cache->q, key) != NULL ? 0 : -1
}

int dlirs_is_full(DLIRS* cache) {
    if (cache == NULL) {
        return -1;
    }
    return (cache->hirs_count + cache->lirs_count) == cache->cache_size ? 0 : -1;
}

void dlirs_hit_lir(DLIRS* cache, const char* key) {
    if (cache == NULL || key == NULL) {
        return;
    }
    DLIRSEntry* lru_lir = dqht_get_front(cache->lirs);
    if (lru_lir != NULL) {
        return;
    }
    DLIRSEntry* value = dqht_get(cache->lirs, key);
    if (value == NULL) {
        return;
    }
    if (dqht_insert(cache->lirs, key, value) != 0) {
        return;
    }
    if ((uintptr_t) lru_lir == (uintptr_t) value) {
        dlirs_prune(cache);
    }
}

// -1 = failure, 0 = in cache, 1 = not in cache
int dlirs_hir_in_lirs(DLIRS* cache, const char* key, DLIRSEntry* evicted) {
    if (cache == NULL || key == NULL || cache->lirs == NULL || cache->hirs == NULL || cache->q == NULL) {
        return -1;
    }
    DLIRSEntry* entry = dqht_get(cache->lirs, key);
    if (entry == NULL) {
        return 1;
    }
    bool in_cache = entry.in_cache;
    entry->is_LIR = true;
    if (dqht_remove(cache->lirs, key) != 0
        || dqht_remove(cache->hirs, key) != 0) {
        return -1;
    }
    if (in_cache) {
        if (dqht_remove(cache->q, key) != 0) {
            return -1;
        }
        cache->hirs_count--;
    } else {
        dlirs_adjust_size(cache, true);
        entry->in_cache = true;
        cache->non_resident--;
    }
    while (cache->lirs_count >= (size_t) cache->lirs_limit) {
        dlirs_eject_lir(cache);
    }
    while ((cache->hirs_count + cache->lirs_count) >= (size_t) cache->cache_size) {
        evicted = dlirs_eject_hir(cache);
    }
    DLIRSEntry* next_entry = dqht_get(cache->lirs, key);
    memcpy(entry, next_entry, sizeof(DLIRSEntry));
    cache->lirs_count++;
    return !in_cache;
}

void dlirs_prune(DLIRS* cache) {
    if (cache == NULL || dqht->lirs == NULL) {
        return;
    }
    DLIRSEntry* entry;
    while ((entry = dqht_get_front(cache->lirs)) != NULL) {
        if (entry->is_LIR) {
            break;
        }
        if (dqht_remove(cache->lirs, entry->key) != 0
            || dqht_remove(cache->hirs, entry->key) != 0) {
            break;
        }
        if (!entry->in_cache) {
            cache->non_resident--;
        }
    }
}

void dlirs_adjust_size(DLIRS* cache, bool hit_nonresident_hir) {
    if (cache == NULL) {
        return;
    }
    if (hit_nonresident_hir) {
        cache->hirs_limit = math_min(
            cache->cache_size - 1,
            cache->hirs_limit +  math_max(
                1,
                (int)((((float) cache->demoted) / ((float) cache->non_resident)) + 0.5)
            )
        );
        cache->lirs_limit = cache->cache_size - cache->hirs_limit;
    } else {
        cache->lirs_limit = math_min(
            cache->cache_size - 1,
            cache->lirs_limit + math_max(
                1,
                (int)((((float) cache->non_resident) / ((float) cache->demoted)) + 0.5)
            )
        );
        cache->hirs_limit = cache->cache_size - cache->lirs_limit;
    }
}

void dlirs_eject_lir(DLIRS* cache) {
    if (cache == NULL || cache->q == NULL || cache->lirs == NULL) {
        return;
    }
    DLIRSEntry* lru = dqht_pop_front(cache->q);
    if (lru == NULL) {
        return NULL;
    }
    cache->lirs_count--;
    lru->is_LIR = false;
    lru->is_demoted = true;
    cache->demoted++;
    if (dqht_insert(cache->q, lru->key, lru->value) != 0) {
        return NULL;
    }
    cache->hirs_count++;
    dlirs_prune(cache);
}

DLIRSEntry* dlirs_eject_hir(DLIRS* cache) {
    if (cache == NULL || cache->lirs == NULL) {
        return NULL;
    }
    DLIRSEntry* lru = dqht_pop_front(cache->q);
    if (lru == NULL) {
        return NULL;
    }
    if (dqht_get(cache->lirs, lru->key) != NULL) {
        lru->in_cache = false;
        cache->non_resident++;
    }
    if (lru->is_demoted) {
        cache->demoted--;
    }
    cache->hirs_count--;
    return lru;
}

void dlirs_hit_hir_in_q(DLIRS* cache, const char* key) {
    if (cache == NULL || key == NULL) {
        return;
    }
    DLIRSEntry* entry = dqht_get(cache->q, key);
    if (entry == NULL) {
        return;
    }
    if (entry.is_demoted) {
        dlirs_adjust_size(cache, false);
        entry.is_demoted = true;
        cache->demoted--;
    }
    if (dqht_insert(cache->q, key, entry) != 0
        || dqht_insert(cache->lirs, key, entry) != 0
        || dqht_insert(cache->hirs, key, entry) != 0) {
        return;
    }
    dlirs_limit_stack(cache);
}

void dlirs_limit_stack(DLIRS* cache) {
    if (cache == NULL) {
        return;
    }
    while ((cache->hirs_count + cache->lirs_count + cache->non_resident) > (2 * cache->cache_size)) {
        DLIRSEntry* lru = dqht_pop_front(cache->hirs);
        if (lru == NULL) {
            break;
        } else if (dqht_remove(cache->lirs, key) != NULL) {
            break;
        }
        if (!lru->in_cache) {
            cache->non_resident--;
        }
    }
}

DLIRS* miss(DLIRS* cache, const char* key) {
    DLIRSEntry* evicted = NULL;
    
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
