#include "dlirs.h"

#include <stddef.h>
#include <sys/mman.h>
#include <stdlib.h>

#include "../../math_utils.h"
#include "../../logging/logging.h"
#include "../../result.h"
#include "../cache_key.h"

DLIRS* dlirs_create(AM_ALLOCATOR_PARAM size_t ht_size, size_t cache_size, DLIRSOptions* options) {
    DLIRS* cache = (DLIRS*) am_malloc(sizeof(*cache));
    if (cache == NULL) {
        return NULL;
    }
    cache->cache_size = cache_size;
    cache->value_copy_handler = options->value_copy_handler;

    cache->hirs_ratio = options != NULL ? options->hirs_ratio : 0.01f; // 0.01f;
    cache->hirs_limit = math_max(1.0f, (int)((cache_size * cache->hirs_ratio) + 0.5f));
    cache->lirs_limit = (float) cache_size - cache->hirs_limit;

    cache->hirs_count = 0;
    cache->lirs_count = 0;
    cache->demoted = 0;
    cache->non_resident = 0;
#define init_dqht(target) ({ \
    (target) = dqht_create(AM_ALLOCATOR_ARG ht_size, options->comparator); \
    if ((target) == NULL) { \
        return NULL; \
    } \
})
    init_dqht(cache->lirs);
    init_dqht(cache->non_resident_hirs);
    init_dqht(cache->resident_hirs);

    return cache;
}

bool dlirs_contains(DLIRS* cache, const char* key) {\
    if (cache == NULL || key == NULL || DLIRS_STRICT_CHECK(cache)) {
        return false;
    }
    DLIRSEntry* entry;
    if ((entry = (DLIRSEntry*) dqht_get(cache->lirs, key)) != NULL && entry->in_cache) {
        return entry->value != NULL;
    }
    return (entry = (DLIRSEntry*) dqht_get(cache->resident_hirs, key)) != NULL ? entry->value != NULL : false;
}

bool dlirs_is_full(DLIRS* cache) {
    if (cache == NULL) {
        return false;
    }
    return (cache->hirs_count + cache->lirs_count) == cache->cache_size;
}

void* dlirs_get(AM_ALLOCATOR_PARAM DLIRS* cache, const char* key, DLIRSEntry** evicted) {
    if (cache == NULL || key == NULL || DLIRS_STRICT_CHECK(cache)) {
        return NULL;
    }
    int miss = 0;
    *evicted = NULL;
    DLIRSEntry* entry = NULL;
    if ((entry = (DLIRSEntry*) dqht_get(cache->lirs, key)) != NULL) {
        if (entry->is_LIR) {
            dlirs_hit_lir(AM_ALLOCATOR_ARG cache, key);
        } else {
            miss = dlirs_hir_in_lirs(AM_ALLOCATOR_ARG cache, key, evicted);
        }
    } else if ((entry = (DLIRSEntry*) dqht_get(cache->resident_hirs, key)) != NULL) {
        dlirs_hit_hir_in_resident_hirs(AM_ALLOCATOR_ARG cache, key);
    }
    return entry;
}

void dlirs_hit_lir(AM_ALLOCATOR_PARAM DLIRS* cache, const char* key) {
    if (cache == NULL ||  key == NULL || DLIRS_STRICT_CHECK(cache)) {
        return;
    }
    DLIRSEntry* lru_lir = (DLIRSEntry*) dqht_get_front(cache->lirs);
    if (lru_lir == NULL) {
        return;
    }
    DLIRSEntry* value = (DLIRSEntry*) dqht_remove(AM_ALLOCATOR_ARG cache->lirs, key);
    if (value == NULL) {
        return;
    }
    if (dqht_insert(AM_ALLOCATOR_ARG cache->lirs, key, value) != 0) {
        return;
    }
    if (key_cmp(lru_lir->key, value->key) == 0) {
        dlirs_prune(AM_ALLOCATOR_ARG cache);
    }
}

// -1 = failure, 0 = in cache, 1 = not in cache
int dlirs_hir_in_lirs(AM_ALLOCATOR_PARAM DLIRS* cache, const char* key, DLIRSEntry** evicted) {
    if (cache == NULL || key == NULL || DLIRS_STRICT_CHECK(cache)) {
        return -1;
    }
    DLIRSEntry* entry = (DLIRSEntry*) dqht_remove(AM_ALLOCATOR_ARG cache->lirs, key);
    if (entry == NULL) {
        return 1;
    }
    bool in_cache = entry->in_cache;
    entry->is_LIR = true;
    DLIRSEntry* hirsEntry;
    if ((hirsEntry = (DLIRSEntry*) dqht_remove(AM_ALLOCATOR_ARG cache->non_resident_hirs, key)) == NULL) {
        am_free(((Result*) entry->value)->results);
        am_free(entry->value);
        dlirs_entry_destroy(AM_ALLOCATOR_ARG entry);
        return -1;
    }
    am_free(((Result*) hirsEntry->value)->results);
    am_free(hirsEntry->value);
    dlirs_entry_destroy(AM_ALLOCATOR_ARG hirsEntry);
    if (in_cache) {
        DLIRSEntry* residentEntry;
        if ((residentEntry = (DLIRSEntry*) dqht_remove(AM_ALLOCATOR_ARG cache->resident_hirs, key)) == NULL) {
            am_free(((Result*) entry->value)->results);
            am_free(entry->value);
            dlirs_entry_destroy(AM_ALLOCATOR_ARG entry);
            return -1;
        }
        cache->hirs_count--;
        am_free(((Result*) residentEntry->value)->results);
        am_free(residentEntry->value);
        dlirs_entry_destroy(AM_ALLOCATOR_ARG residentEntry);
    } else {
        entry->in_cache = true;
        DLIRSEntry* residentEntry;
        if ((residentEntry = (DLIRSEntry*) dqht_get(cache->resident_hirs, key)) != NULL) {
            residentEntry->is_LIR = true;
        }
        dlirs_resize(cache, true);
        if (residentEntry != NULL) {
            residentEntry->in_cache = true;
        }
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
    while (cache->lirs->ht->count > 0) {
        entry = (DLIRSEntry*) dqht_get_front(cache->lirs);
        if (entry->is_LIR) {
            break;
        }
        bool inCache = entry->in_cache;
        entry2 = (DLIRSEntry*) dqht_remove(AM_ALLOCATOR_ARG cache->non_resident_hirs, entry->key);
        entry1 = (DLIRSEntry*) dqht_remove(AM_ALLOCATOR_ARG cache->lirs, entry->key);
        if (!inCache) {
            cache->non_resident--;
        }
        if (entry2 != NULL) {
            am_free(((Result*) entry2->value)->results);
            am_free(entry2->value);
            dlirs_entry_destroy(AM_ALLOCATOR_ARG entry2);
        }
        if (entry1 != NULL) {
            am_free(((Result*) entry1->value)->results);
            am_free(entry1->value);
            dlirs_entry_destroy(AM_ALLOCATOR_ARG entry1);
        }
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
    DLIRSEntry* lru = (DLIRSEntry*) dqht_pop_front(AM_ALLOCATOR_ARG cache->lirs);
    if (lru == NULL) {
        return;
    }
    cache->lirs_count--;
    lru->is_LIR = false;
    lru->is_demoted = true;
    DLIRSEntry* nonResidentHirsEntry;
    if ((nonResidentHirsEntry = (DLIRSEntry*) dqht_get(cache->non_resident_hirs, lru->key)) != NULL) {
        nonResidentHirsEntry->is_LIR = false;
        nonResidentHirsEntry->is_demoted = true;
    }
    cache->demoted++;
    DLIRSEntry* residentHirsEntry;
    if ((residentHirsEntry = (DLIRSEntry*) dqht_remove(AM_ALLOCATOR_ARG cache->resident_hirs, lru->key)) != NULL) {
        am_free(((Result*) residentHirsEntry->value)->results);
        am_free(residentHirsEntry->value);
        dlirs_entry_destroy(AM_ALLOCATOR_ARG residentHirsEntry);
    }
    if (dqht_insert(AM_ALLOCATOR_ARG cache->resident_hirs, lru->key, lru) != 0) {
        return;
    }
    cache->hirs_count++;
    dlirs_prune(AM_ALLOCATOR_ARG cache);
}

DLIRSEntry* dlirs_evict_resident_hir(AM_ALLOCATOR_PARAM DLIRS* cache) {
    if (cache == NULL || DLIRS_STRICT_CHECK(cache)) {
        return NULL;
    }
    DLIRSEntry* lru = (DLIRSEntry*) dqht_pop_front(AM_ALLOCATOR_ARG cache->resident_hirs);
    if (lru == NULL) {
        return NULL;
    }
    DLIRSEntry* entry;
    if ((entry = (DLIRSEntry*) dqht_get(cache->lirs, lru->key)) != NULL) {
        lru->in_cache = false;
        entry->in_cache = false;
        if ((entry = (DLIRSEntry*) dqht_get(cache->non_resident_hirs, lru->key)) != NULL) {
            entry->in_cache = false;
        }
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
    DLIRSEntry* entry = (DLIRSEntry*) dqht_get(cache->resident_hirs, key);
    if (entry == NULL) {
        return;
    }
    if (entry->is_demoted) {
        dlirs_resize(cache, false);
        entry->is_demoted = false;
        cache->demoted--;
    }
    DLIRSEntry* newEntry = dlirs_entry_copy(AM_ALLOCATOR_ARG entry, cache->value_copy_handler);
    DLIRSEntry* removeEntry;
    if ((removeEntry = (DLIRSEntry*) dqht_remove(AM_ALLOCATOR_ARG cache->resident_hirs, key)) != NULL) {
        am_free(((Result*) removeEntry->value)->results);
        am_free(removeEntry->value);
        dlirs_entry_destroy(AM_ALLOCATOR_ARG removeEntry);
    }
    if ((removeEntry = (DLIRSEntry*) dqht_remove(AM_ALLOCATOR_ARG cache->lirs, key)) != NULL) {
        am_free(((Result*) removeEntry->value)->results);
        am_free(removeEntry->value);
        dlirs_entry_destroy(AM_ALLOCATOR_ARG removeEntry);
    }
    if ((removeEntry = (DLIRSEntry*) dqht_remove(AM_ALLOCATOR_ARG cache->non_resident_hirs, key)) != NULL) {
        am_free(((Result*) removeEntry->value)->results);
        am_free(removeEntry->value);
        dlirs_entry_destroy(AM_ALLOCATOR_ARG removeEntry);
    }
    if (dqht_insert(AM_ALLOCATOR_ARG cache->resident_hirs, key, newEntry) != 0
        || dqht_insert(AM_ALLOCATOR_ARG cache->lirs, key, dlirs_entry_copy(AM_ALLOCATOR_ARG newEntry, cache->value_copy_handler)) != 0
        || dqht_insert(AM_ALLOCATOR_ARG cache->non_resident_hirs, key, dlirs_entry_copy(AM_ALLOCATOR_ARG newEntry, cache->value_copy_handler)) != 0) {
        return;
    }
    dlirs_limit_stack(AM_ALLOCATOR_ARG cache);
}

void dlirs_limit_stack(AM_ALLOCATOR_PARAM DLIRS* cache) {
    if (cache == NULL || DLIRS_STRICT_CHECK(cache)) {
        return;
    }
    while ((cache->hirs_count + cache->lirs_count + cache->non_resident) > (2 * cache->cache_size)) {
        DLIRSEntry* lru = (DLIRSEntry*) dqht_pop_front(AM_ALLOCATOR_ARG cache->non_resident_hirs);
        if (!lru->in_cache) {
            cache->non_resident--;
        }
        DLIRSEntry* entry;
        if ((entry = (DLIRSEntry*) dqht_remove(AM_ALLOCATOR_ARG cache->lirs, lru->key)) != NULL) {
            am_free(((Result*) entry->value)->results);
            am_free(entry->value);
            dlirs_entry_destroy(AM_ALLOCATOR_ARG entry);
        }
        am_free(((Result*) lru->value)->results);
        am_free(lru->value);
        dlirs_entry_destroy(AM_ALLOCATOR_ARG lru);
    }
}

int dlirs_request(AM_ALLOCATOR_PARAM DLIRS* cache, const char* key, void* value, DLIRSEntry** evicted) {
    if (cache == NULL || key == NULL || DLIRS_STRICT_CHECK(cache)) {
        return -1;
    }
    if (cache->lirs_count < (size_t) cache->lirs_limit && cache->hirs_count == 0) {
        DLIRSEntry* entry = dlirs_entry_create(AM_ALLOCATOR_ARG key, value);
        if (entry == NULL) {
            return -1;
        }
        entry->is_LIR = true;
        if (dqht_insert(AM_ALLOCATOR_ARG cache->lirs, key, entry) != 0) {
            return -1;
        }
        cache->lirs_count++;
        *evicted = NULL;
        return 0;
    }
    while ((cache->hirs_count + cache->lirs_count) >= cache->cache_size) {
        while (cache->lirs_count > cache->lirs_limit) {
            dlirs_evict_lir(AM_ALLOCATOR_ARG cache);
        }
        *evicted = dlirs_evict_resident_hir(AM_ALLOCATOR_ARG cache);
    }
    DLIRSEntry* entry = dlirs_entry_create(AM_ALLOCATOR_ARG key, value);
    if (entry == NULL
        || dqht_insert(AM_ALLOCATOR_ARG cache->lirs, key, entry) != 0
        || dqht_insert(AM_ALLOCATOR_ARG cache->non_resident_hirs, key, dlirs_entry_copy(AM_ALLOCATOR_ARG entry, cache->value_copy_handler)) != 0
        || dqht_insert(AM_ALLOCATOR_ARG cache->resident_hirs, key, dlirs_entry_copy(AM_ALLOCATOR_ARG entry, cache->value_copy_handler)) != 0) {
        return -1;
    }
    cache->hirs_count++;
    dlirs_limit_stack(AM_ALLOCATOR_ARG cache);
    return 0;
}

void destroy_hirs(AM_ALLOCATOR_PARAM DLIRSEntry* entry, void* _ignored) {
    if (entry == NULL || entry->key == NULL) {
        return;
    }
    am_free(((Result*) entry->value)->results);
    am_free(entry->value);
    dlirs_entry_destroy(AM_ALLOCATOR_ARG entry);
}

void destroy_resident_hirs(AM_ALLOCATOR_PARAM DLIRSEntry* entry, void* _ignored) {
    if (entry == NULL || entry->key == NULL) {
        return;
    }
    am_free(((Result*) entry->value)->results);
    am_free(entry->value);
    dlirs_entry_destroy(AM_ALLOCATOR_ARG entry);
}

void destroy_lirs(AM_ALLOCATOR_PARAM DLIRSEntry* entry, void* _ignored) {
    if (entry == NULL || entry->key == NULL) {
        return;
    }
    am_free(((Result*) entry->value)->results);
    am_free(entry->value);
    dlirs_entry_destroy(AM_ALLOCATOR_ARG entry);
}

int dlirs_destroy(AM_ALLOCATOR_PARAM DLIRS* cache) {
    if (cache == NULL) {
        return 0;
    }
    dqht_destroy_handled(
        AM_ALLOCATOR_ARG
        cache->resident_hirs,
        (EntryValueDestroyHandler) destroy_hirs,
        cache
    );
    DEBUG("[Cache: %p, Table: %p] Destroyed Resident HIRS table", cache, cache->resident_hirs);
    dqht_destroy_handled(
        AM_ALLOCATOR_ARG
        cache->non_resident_hirs,
        (EntryValueDestroyHandler) destroy_resident_hirs,
        cache
    );
    DEBUG("[Cache: %p, Table: %p] Destroyed Non-Resident HIRS table", cache, cache->non_resident_hirs);
    dqht_destroy_handled(
        AM_ALLOCATOR_ARG
        cache->lirs,
        (EntryValueDestroyHandler) destroy_lirs,
        NULL
    );
    DEBUG("[Cache: %p, Table: %p] Destroyed LIRS table", cache, cache->lirs);
    am_free(cache);
    return 0;
}