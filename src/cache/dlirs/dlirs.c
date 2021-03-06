#include "dlirs.h"

#include <stddef.h>
#include <sys/mman.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../math_utils.h"
#include "../../logging/logging.h"
#include "../preprocessor/lambda.h"

DLIRS* dlirs_create(AM_ALLOCATOR_PARAM size_t ht_size, size_t cache_size, DLIRSOptions* options) {
    DLIRS* cache = am_malloc(sizeof(*cache));
    if (cache == NULL) {
        return NULL;
    }
    cache->cache_size = cache_size;

    cache->hirs_ratio = options != NULL ? options->hirs_ratio : 0.01f; // 0.01f;
    cache->hirs_limit = math_max(1.0f, (int)((cache_size * cache->hirs_ratio) + 0.5f));
    cache->lirs_limit = (float) cache_size - cache->hirs_limit;

    cache->hirs_count = 0;
    cache->lirs_count = 0;
    cache->demoted = 0;
    cache->non_resident = 0;
#define init_dqht(target) ({ \
    (target) = dqht_create(AM_ALLOCATOR_ARG ht_size, options == NULL ? NULL : options->comparator); \
    if ((target) == NULL) { \
        return NULL; \
    } \
})
    init_dqht(cache->lirs);
    init_dqht(cache->non_resident_hirs);
    init_dqht(cache->resident_hirs);

    return cache;
}

bool dlirs_contains(DLIRS* cache, const char* key) {
    return dlirs_get(cache, key) != NULL;
}

bool dlirs_is_full(DLIRS* cache) {
    if (cache == NULL) {
        return false;
    }
    return (cache->hirs_count + cache->lirs_count) == cache->cache_size;
}

void* dlirs_get(DLIRS* cache, const char* key) {
    if (cache == NULL || key == NULL || DLIRS_STRICT_CHECK(cache)) {
        return false;
    }
    DLIRSEntry* value;
    if ((value = dqht_get(cache->lirs, key)) != NULL && value->in_cache) {
        return value->value;
    }
    return (value = dqht_get(cache->resident_hirs, key)) != NULL ? value->value : NULL;
}

void dlirs_hit_lir(AM_ALLOCATOR_PARAM DLIRS* cache, const char* key) {
    if (cache == NULL ||  key == NULL || DLIRS_STRICT_CHECK(cache)) {
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
    if (dqht_insert(AM_ALLOCATOR_ARG cache->lirs, key, value) != 0) {
        return;
    }
    if ((uintptr_t) lru_lir == (uintptr_t) value) {
        dlirs_prune(AM_ALLOCATOR_ARG cache);
    }
}

// -1 = failure, 0 = in cache, 1 = not in cache
int dlirs_hir_in_lirs(AM_ALLOCATOR_PARAM DLIRS* cache, const char* key, DLIRSEntry** evicted) {
    if (cache == NULL || key == NULL || DLIRS_STRICT_CHECK(cache)) {
        return -1;
    }
    DLIRSEntry* entry = dqht_get(cache->lirs, key);
    if (entry == NULL) {
        return 1;
    }
    bool in_cache = entry->in_cache;
    entry->is_LIR = true;
    DLIRSEntry* hirsEntry;
    if (dqht_remove(AM_ALLOCATOR_ARG cache->lirs, key) == NULL
        || (hirsEntry = dqht_remove(AM_ALLOCATOR_ARG cache->non_resident_hirs, key)) == NULL) {
        dlirs_entry_destroy(AM_ALLOCATOR_ARG entry);
        return -1;
    }
    dlirs_entry_destroy(AM_ALLOCATOR_ARG hirsEntry);
    if (in_cache) {
        DLIRSEntry* residentEntry;
        if ((residentEntry = dqht_remove(AM_ALLOCATOR_ARG cache->resident_hirs, key)) == NULL) {
            return -1;
        }
        cache->hirs_count--;
        dlirs_entry_destroy(AM_ALLOCATOR_ARG residentEntry);
    } else {
        dlirs_resize(cache, true);
        entry->in_cache = true;
        cache->non_resident--;
    }
    TRACE("Before eject lir loop");
    while (cache->lirs_count >= (size_t) cache->lirs_limit) {
        TRACE("Ejecting LIR: [Count: %zu] [Limit: %f]", cache->lirs_count, cache->lirs_limit);
        dlirs_evict_lir(AM_ALLOCATOR_ARG cache);
    }
    TRACE("After eject lir loop");
    TRACE("Before eject hir loop");
    while ((cache->hirs_count + cache->lirs_count) >= (size_t) cache->cache_size) {
        TRACE("Ejecting HIR: [Count: %zu] [Limit: %zu]", (cache->hirs_count + cache->lirs_count), cache->cache_size);
        *evicted = dlirs_evict_resident_hir(AM_ALLOCATOR_ARG cache);
    }
    TRACE("After eject hir loop");
    dqht_insert(AM_ALLOCATOR_ARG cache->lirs, key, entry);
    cache->lirs_count++;
    return !in_cache;
}

void dlirs_prune(AM_ALLOCATOR_PARAM DLIRS* cache) {
    if (cache == NULL || DLIRS_STRICT_CHECK(cache)) {
        return;
    }
    DLIRSEntry* entry;
    DLIRSEntry* entry1;
    DLIRSEntry* entry2;
    while ((entry = dqht_get_front(cache->lirs)) != NULL) {
        TRACE("Prune LIR entry %p", entry);
        if (entry->is_LIR || (entry1 = dqht_remove(AM_ALLOCATOR_ARG cache->lirs, entry->key)) == NULL) {
            break;
        } else if ((entry2 = dqht_remove(AM_ALLOCATOR_ARG cache->non_resident_hirs, entry->key)) == NULL) {
            dlirs_entry_destroy(AM_ALLOCATOR_ARG entry1);
            break;
        }
        TRACE("Prune removed [LIR: %p] [HIR: %p]", entry1, entry2);
        if (!entry->in_cache) {
            cache->non_resident--;
        }
        dlirs_entry_destroy(AM_ALLOCATOR_ARG entry1);
        dlirs_entry_destroy(AM_ALLOCATOR_ARG entry2);
        TRACE("Destroyed hir-lir");
    }
}

void dlirs_resize(DLIRS* cache, bool hit_nonresident_hir) {
    if (cache == NULL || DLIRS_STRICT_CHECK(cache)) {
        return;
    } else if (hit_nonresident_hir) {
        cache->hirs_limit = math_min(
            cache->cache_size - 1,
            cache->hirs_limit +  math_max(
                1,
                (int)((((float) cache->demoted) / ((float) cache->non_resident)) + 0.5)
            )
        );
        cache->lirs_limit = (float) cache->cache_size - cache->hirs_limit;
        return;
    }
    cache->lirs_limit = math_min(
        cache->cache_size - 1,
        cache->lirs_limit + math_max(
            1,
            (int)((((float) cache->non_resident) / ((float) cache->demoted)) + 0.5)
        )
    );
    cache->hirs_limit = (float) cache->cache_size - cache->lirs_limit;
}

void dlirs_evict_lir(AM_ALLOCATOR_PARAM DLIRS* cache) {
    if (cache == NULL || DLIRS_STRICT_CHECK(cache)) {
        return;
    }
    DLIRSEntry* lru = dqht_pop_front(AM_ALLOCATOR_ARG cache->lirs);
    if (lru == NULL) {
        return;
    }
    cache->lirs_count--;
    lru->is_LIR = false;
    lru->is_demoted = true;
    cache->demoted++;
    if (dqht_insert(AM_ALLOCATOR_ARG cache->resident_hirs, lru->key, dlirs_entry_copy(AM_ALLOCATOR_ARG lru)) != 0) {
        return;
    }
    dlirs_entry_destroy(AM_ALLOCATOR_ARG lru);
    cache->hirs_count++;
    TRACE("Incremented HIRS count");
    dlirs_prune(AM_ALLOCATOR_ARG cache);
}

DLIRSEntry* dlirs_evict_resident_hir(AM_ALLOCATOR_PARAM DLIRS* cache) {
    if (cache == NULL || DLIRS_STRICT_CHECK(cache)) {
        return NULL;
    }
    DLIRSEntry* lru = dqht_pop_front(AM_ALLOCATOR_ARG cache->resident_hirs);
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

void dlirs_hit_hir_in_resident_hirs(AM_ALLOCATOR_PARAM DLIRS* cache, const char* key) {
    if (cache == NULL || key == NULL || DLIRS_STRICT_CHECK(cache)) {
        return;
    }
    DLIRSEntry* entry = dqht_get(cache->resident_hirs, key);
    if (entry == NULL) {
        return;
    }
    if (entry->is_demoted) {
        dlirs_resize(cache, false);
        entry->is_demoted = true;
        cache->demoted--;
    }
    if (dqht_insert(AM_ALLOCATOR_ARG cache->resident_hirs, key, entry) != 0
        || dqht_insert(AM_ALLOCATOR_ARG cache->lirs, key, dlirs_entry_copy(AM_ALLOCATOR_ARG entry)) != 0
        || dqht_insert(AM_ALLOCATOR_ARG cache->non_resident_hirs, key, dlirs_entry_copy(AM_ALLOCATOR_ARG entry)) != 0) {
        return;
    }
    dlirs_limit_stack(AM_ALLOCATOR_ARG cache);
}

void dlirs_limit_stack(AM_ALLOCATOR_PARAM DLIRS* cache) {
    if (cache == NULL || DLIRS_STRICT_CHECK(cache)) {
        return;
    }
    while ((cache->hirs_count + cache->lirs_count + cache->non_resident) > (2 * cache->cache_size)) {
        DLIRSEntry* lru = dqht_pop_front(AM_ALLOCATOR_ARG cache->non_resident_hirs);
        DLIRSEntry* entry;
        if (lru == NULL) {
            break;
        } else if ((entry = dqht_remove(AM_ALLOCATOR_ARG cache->lirs, lru->key)) == NULL) {
            dlirs_entry_destroy(AM_ALLOCATOR_ARG lru);
            break;
        }
        if (!lru->in_cache) {
            cache->non_resident--;
        }
        dlirs_entry_destroy(AM_ALLOCATOR_ARG lru);
        dlirs_entry_destroy(AM_ALLOCATOR_ARG entry);
    }
}

int dlirs_miss(AM_ALLOCATOR_PARAM DLIRS* cache, const char* key, void* value, DLIRSEntry** evicted) {
    if (cache == NULL || key == NULL || DLIRS_STRICT_CHECK(cache)) {
        return -1;
    }
    if (cache->lirs_count < (unsigned long long) cache->lirs_limit && cache->hirs_count == 0) {
        TRACE("MISS ENTRY: %s", key);
        DLIRSEntry* entry = dlirs_entry_create(AM_ALLOCATOR_ARG key, value);
        if (entry == NULL) {
            return -1;
        }
        TRACE("Setting is_LIR true");
        entry->is_LIR = true;
        if (dqht_insert(AM_ALLOCATOR_ARG cache->lirs, key, entry) != 0) {
            return -1;
        }
        cache->lirs_count++;
        *evicted = NULL;
        return 0;
    }
    while ((cache->hirs_count + cache->lirs_count) >= cache->cache_size) {
        while (cache->lirs_count > (unsigned long long) cache->lirs_limit) {
            dlirs_evict_lir(AM_ALLOCATOR_ARG cache);
        }
        *evicted = dlirs_evict_resident_hir(AM_ALLOCATOR_ARG cache);
    }
    TRACE("MISS ENTRY 2: %s", key);
    DLIRSEntry* entry = dlirs_entry_create(AM_ALLOCATOR_ARG key, value);
    if (entry == NULL
        || dqht_insert(AM_ALLOCATOR_ARG cache->resident_hirs, key, entry) != 0
        || dqht_insert(AM_ALLOCATOR_ARG cache->lirs, key, dlirs_entry_copy(AM_ALLOCATOR_ARG entry)) != 0
        || dqht_insert(AM_ALLOCATOR_ARG cache->non_resident_hirs, key, dlirs_entry_copy(AM_ALLOCATOR_ARG entry)) != 0) {
        return -1;
    }
    cache->hirs_count++;
    dlirs_limit_stack(AM_ALLOCATOR_ARG cache);
    return 0;
}

// -1 = failure, 0 = miss, 1 = hit
int dlirs_request(AM_ALLOCATOR_PARAM DLIRS* cache, const char* key, void* value, DLIRSEntry** evicted) {
    if (cache == NULL || key == NULL || DLIRS_STRICT_CHECK(cache)) {
        return -1;
    }
    int miss = 0;
    *evicted = NULL;

    DLIRSEntry* entry = dqht_get(cache->lirs, key);
    TRACE("Entry retrieval reached %s, %p", key, entry);
    if (entry != NULL) {
        if (entry->is_LIR) {
            TRACE("Hit LIR before");
            dlirs_hit_lir(AM_ALLOCATOR_ARG cache, key);
            TRACE("Hit LIR after");
        } else {
            TRACE("Hit HIR in LIRS before");
            miss = dlirs_hir_in_lirs(AM_ALLOCATOR_ARG cache, key, evicted);
            TRACE("Hit HIR in LIRS after");
        }
    } else if (dqht_get(cache->resident_hirs, key) != NULL) {
        TRACE("Hit HIR in Q before");
        dlirs_hit_hir_in_resident_hirs(AM_ALLOCATOR_ARG cache, key);
        TRACE("Hit HIR in Q after");
    } else {
        miss = 1;
        TRACE("Miss before");
        if (dlirs_miss(AM_ALLOCATOR_ARG cache, key, value, evicted) != 0) {
            return -1;
        }
        TRACE("Miss after");
    }
    return !miss;
}

int dlirs_destroy(AM_ALLOCATOR_PARAM DLIRS* cache) {
    if (cache == NULL) {
        return 0;
    }
    dqht_destroy_handled(
        AM_ALLOCATOR_ARG
        cache->resident_hirs,
        (EntryValueDestroyHandler) lambda(void, (AM_ALLOCATOR_PARAM DLIRSEntry* entry, DLIRS* dlirs), {
            if (entry == NULL || entry->key == NULL) {
                return;
            } else if (dqht_get(dlirs->lirs, entry->key) == NULL
                       && dqht_get(dlirs->non_resident_hirs, entry->key) == NULL) {
                dlirs_entry_destroy(AM_ALLOCATOR_ARG entry);
            }
        }),
        cache
    );
    DEBUG("[Cache: %p, Table: %p] Destroyed Resident HIRS table", cache, cache->resident_hirs);
    dqht_destroy_handled(
        AM_ALLOCATOR_ARG
        cache->non_resident_hirs,
        (EntryValueDestroyHandler) lambda(void, (AM_ALLOCATOR_PARAM DLIRSEntry* entry, DLIRS* dlirs), {
            if (entry == NULL || entry->key == NULL) {
                return;
            } else if (dqht_get(dlirs->lirs, entry->key) == NULL) {
                dlirs_entry_destroy(AM_ALLOCATOR_ARG entry);
            }
        }),
        cache
    );
    DEBUG("[Cache: %p, Table: %p] Destroyed Non-Resident HIRS table", cache, cache->non_resident_hirs);
    dqht_destroy_handled(
        AM_ALLOCATOR_ARG
        cache->lirs,
        (EntryValueDestroyHandler) lambda(void, (AM_ALLOCATOR_PARAM DLIRSEntry* entry, void* _ignored), {
            if (entry == NULL || entry->key == NULL) {
            return;
        }
            dlirs_entry_destroy(AM_ALLOCATOR_ARG entry);
        }),
        NULL
    );
    DEBUG("[Cache: %p, Table: %p] Destroyed LIRS table", cache, cache->lirs);
    am_free(cache);
    return 0;
}