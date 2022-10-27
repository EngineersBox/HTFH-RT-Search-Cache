#include "dlirs.h"

#include <stddef.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <math.h>

#include "../../logging/logging.h"
#include "../../result.h"
#include "../cache_key.h"

void entry_destroy_handler(AM_ALLOCATOR_PARAM void* entry, void* currentValue) {
    DLIRSEntry* dlirsEntry = (DLIRSEntry*) entry;
    if (dlirsEntry == NULL || dlirsEntry->key == NULL) {
        return;
    }
    rc_decrement(dlirsEntry->reference_counter);
    char* keyValue = key_sprint(dlirsEntry->key);
    TRACE("DLIRSEntry %s reference counter decremented %d -> %d", keyValue, dlirsEntry->reference_counter + 1, dlirsEntry->reference_counter);
    free(keyValue);
    if (!rc_last(dlirsEntry->reference_counter)) {
        return;
    }
    keyValue = key_sprint(dlirsEntry->key);
    TRACE("DESTROYING DLIRSEntry: %p [%s: %p]", dlirsEntry, keyValue, dlirsEntry->value);
    free(keyValue);
    am_free(dlirsEntry->key);
    dlirsEntry->key = NULL;
    if ((intptr_t) currentValue != (intptr_t) dlirsEntry->value) {
        dlirsEntry->value_destroy_handler(AM_ALLOCATOR_ARG dlirsEntry->value);
    }
    am_free(dlirsEntry);
}

DLIRS* dlirs_create(AM_ALLOCATOR_PARAM size_t ht_size, size_t cache_size, DLIRSOptions* options) {
    DLIRS* cache = (DLIRS*) am_malloc(sizeof(*cache));
    if (cache == NULL) {
        return NULL;
    }
    cache->cache_size = cache_size;
    cache->value_copy_handler = options->value_copy_handler;

    cache->hirs_ratio = options != NULL ? options->hirs_ratio : 0.01f; // 0.01f;
    cache->hirs_limit = (float) fmax(1.0f, rint(((double) cache_size * cache->hirs_ratio) + 0.5));
    cache->lirs_limit = (float) cache_size - cache->hirs_limit;

    cache->hirs_count = 0;
    cache->lirs_count = 0;
    cache->demoted = 0;
    cache->non_resident = 0;
#define init_dqht(target) ({ \
    (target) = dqht_create(AM_ALLOCATOR_ARG ht_size, options->comparator, (EntryValueDestroyHandler) entry_destroy_handler); \
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
    return miss == 1 ? NULL : entry;
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
    } else if (dqht_insert(AM_ALLOCATOR_ARG cache->lirs, value->key, value) != 0) {
        FATAL("Bad hit LIR re-insert");
        return;
    } else if (key_cmp(lru_lir->key, key) == 0) {
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
        dlirs_entry_destroy(AM_ALLOCATOR_ARG entry);
        return -1;
    }
    dlirs_entry_destroy(AM_ALLOCATOR_ARG hirsEntry);
    if (in_cache) {
        DLIRSEntry* residentEntry;
        if ((residentEntry = (DLIRSEntry*) dqht_remove(AM_ALLOCATOR_ARG cache->resident_hirs, key)) == NULL) {
            dlirs_entry_destroy(AM_ALLOCATOR_ARG entry);
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
        TRACE("Ejecting LIR: [Count: %zu] [Limit: %zu]", cache->lirs_count, (size_t) cache->lirs_limit);
        dlirs_evict_lir(AM_ALLOCATOR_ARG cache);
    }
    TRACE("After eject lir loop");
    TRACE("Before eject hir loop");
    while ((cache->hirs_count + cache->lirs_count) >= (size_t) cache->cache_size) {
        TRACE("Ejecting HIR: [Count: %zu] [Limit: %zu]", (cache->hirs_count + cache->lirs_count), cache->cache_size);
        *evicted = dlirs_evict_resident_hir(AM_ALLOCATOR_ARG cache);
    }
    TRACE("After eject hir loop");
    if (dqht_insert(AM_ALLOCATOR_ARG cache->lirs, key, entry) != 0) {
        FATAL("Bad HIR in LIRS reinsert");
    }
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
        DEBUG("Pruning %d > 0", cache->lirs->ht->count);
        dqht_print_table("Pruning", cache->lirs);
        entry = (DLIRSEntry*) dqht_get_front(cache->lirs);
        if (entry->is_LIR) {
            break;
        }
        bool inCache = entry->in_cache;
        entry1 = (DLIRSEntry*) dqht_remove(AM_ALLOCATOR_ARG cache->lirs, entry->key);
        entry2 = (DLIRSEntry*) dqht_remove(AM_ALLOCATOR_ARG cache->non_resident_hirs, entry->key);
        if (!inCache) {
            cache->non_resident--;
        }
        if (entry2 != NULL) {
            dlirs_entry_destroy(AM_ALLOCATOR_ARG entry2);
        }
        if (entry1 != NULL) {
            dlirs_entry_destroy(AM_ALLOCATOR_ARG entry1);
        }
    }
}

void dlirs_resize(DLIRS* cache, bool hit_nonresident_hir) {
    if (hit_nonresident_hir) {
        cache->hirs_limit = (float) fmin(
            (double) cache->cache_size - 1,
            cache->hirs_limit + fmax(1, rint(((double) cache->demoted / (double) cache->non_resident) + 0.5))
        );
        cache->lirs_limit = (float) cache->cache_size - cache->hirs_limit;
    } else {
        cache->lirs_limit = (float) fmin(
            (double) cache->cache_size - 1,
            cache->lirs_limit + fmax(1, rint(((double) cache->non_resident / (double) cache->demoted) + 0.5))
        );
        cache->hirs_limit = (float) cache->cache_size - cache->lirs_limit;
    }
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
    cache->demoted++;
    if (dqht_insert(AM_ALLOCATOR_ARG cache->resident_hirs, lru->key, lru) != 0) {
        FATAL("Bad LIRS re-insert");
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
    DLIRSEntry* entry = (DLIRSEntry*) dqht_get(cache->resident_hirs, key);
    if (entry == NULL) {
        return;
    }
    if (entry->is_demoted) {
        dlirs_resize(cache, false);
        entry->is_demoted = false;
        cache->demoted--;
    }
    DLIRSEntry* lirs_entry = dlirs_entry_reference_copy(AM_ALLOCATOR_ARG entry);
    DLIRSEntry* non_resident_entry = dlirs_entry_reference_copy(AM_ALLOCATOR_ARG entry);
    DEBUG("TABLES %p %p %p", cache->resident_hirs, cache->lirs, cache->non_resident_hirs);
    if (dqht_insert(AM_ALLOCATOR_ARG cache->resident_hirs, key, entry) != 0
        || dqht_insert(AM_ALLOCATOR_ARG cache->lirs, key, lirs_entry) != 0
        || dqht_insert(AM_ALLOCATOR_ARG cache->non_resident_hirs, key, non_resident_entry) != 0) {
        FATAL("Bad reinsert of resident HIR entry");
    }
    dqht_print_table("HIT HIR IN RES HIRS BEFORE LIMIT", cache->non_resident_hirs);
    dlirs_limit_stack(AM_ALLOCATOR_ARG cache);
}

void dlirs_limit_stack(AM_ALLOCATOR_PARAM DLIRS* cache) {
    if (cache == NULL || DLIRS_STRICT_CHECK(cache)) {
        return;
    }
    while ((cache->hirs_count + cache->lirs_count + cache->non_resident) > (2 * cache->cache_size)) {
        DEBUG("Limiting stack %zu > %zu", (cache->hirs_count + cache->lirs_count + cache->non_resident), (2 * cache->cache_size));
        DLIRSEntry* lru = (DLIRSEntry*) dqht_pop_front(AM_ALLOCATOR_ARG cache->non_resident_hirs);
        DEBUG("DLIRSEntry: %p", lru);
        if (!lru->in_cache) {
            cache->non_resident--;
        }
        DLIRSEntry* entry;
        if ((entry = (DLIRSEntry*) dqht_remove(AM_ALLOCATOR_ARG cache->lirs, lru->key)) != NULL) {
            dlirs_entry_destroy(AM_ALLOCATOR_ARG entry);
        }
        dlirs_entry_destroy(AM_ALLOCATOR_ARG lru);
    }
}

int dlirs_request(AM_ALLOCATOR_PARAM DLIRS* cache, const char* key, void* value, DLIRSEntry** evicted) {
    if (cache == NULL || key == NULL || DLIRS_STRICT_CHECK(cache)) {
        return -1;
    }
    if (cache->lirs_count < (size_t) cache->lirs_limit && cache->hirs_count == 0) {
        DLIRSEntry* entry = dlirs_entry_create(AM_ALLOCATOR_ARG key, value, (ValueDestroy) result_destroy);
        if (entry == NULL) {
            return -1;
        }
        entry->is_LIR = true;
        if (dqht_insert(AM_ALLOCATOR_ARG cache->lirs, key, entry) != 0) {
            FATAL("Bad request LIRS insert");
            return -1;
        }
        cache->lirs_count++;
        *evicted = NULL;
        return 0;
    }
    while ((cache->hirs_count + cache->lirs_count) >= cache->cache_size) {
        while (cache->lirs_count > cache->lirs_limit) {
            DEBUG("Evicting LIR %zu > %f", cache->lirs_count, cache->lirs_limit);
            dlirs_evict_lir(AM_ALLOCATOR_ARG cache);
        }
        DEBUG("Evicting resident HIR %zu >= %zu", (cache->hirs_count + cache->lirs_count), cache->cache_size);
        *evicted = dlirs_evict_resident_hir(AM_ALLOCATOR_ARG cache);
    }
    DLIRSEntry* lirs_entry = dlirs_entry_create(AM_ALLOCATOR_ARG key, value, (ValueDestroy) result_destroy);
    DLIRSEntry* non_resident_entry = dlirs_entry_reference_copy(AM_ALLOCATOR_ARG lirs_entry);
    DLIRSEntry* resident_entry = dlirs_entry_reference_copy(AM_ALLOCATOR_ARG lirs_entry);
    if (lirs_entry == NULL
        || dqht_insert(AM_ALLOCATOR_ARG cache->lirs, key, lirs_entry) != 0
        || dqht_insert(AM_ALLOCATOR_ARG cache->non_resident_hirs, key, non_resident_entry) != 0
        || dqht_insert(AM_ALLOCATOR_ARG cache->resident_hirs, key, resident_entry) != 0) {
        FATAL("Bad request full saturation insert");
        return -1;
    }
    cache->hirs_count++;
    dqht_print_table("REQUEST BEFORE LIMIT", cache->non_resident_hirs);
    dlirs_limit_stack(AM_ALLOCATOR_ARG cache);
    return 0;
}

void destroy_hirs(AM_ALLOCATOR_PARAM DLIRSEntry* entry, void* _ignored) {
    if (entry == NULL || entry->key == NULL) {
        return;
    }
    dlirs_entry_destroy(AM_ALLOCATOR_ARG entry);
}

void destroy_resident_hirs(AM_ALLOCATOR_PARAM DLIRSEntry* entry, void* _ignored) {
    if (entry == NULL || entry->key == NULL) {
        return;
    }
    dlirs_entry_destroy(AM_ALLOCATOR_ARG entry);
}

void destroy_lirs(AM_ALLOCATOR_PARAM DLIRSEntry* entry, void* _ignored) {
    if (entry == NULL || entry->key == NULL) {
        return;
    }
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