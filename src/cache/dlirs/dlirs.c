#include "dlirs.h"

#include <stddef.h>
#include <sys/mman.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../math_utils.h"
#define ENABLE_LOGGING
#define LOG_DATETIME_PREFIX
#include "../../logging/logging.h"
#include "../preprocessor/lambda.h"

DLIRS* dlirs_create(size_t ht_size, size_t cache_size, float hirs_ratio) {
    DLIRS* cache = malloc(sizeof(*cache));
    if (cache == NULL) {
        return NULL;
    }
    cache->cache_size = cache_size;

    cache->hirs_ratio = hirs_ratio; // 0.01f;
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

    return cache;
}

bool dlirs_contains(DLIRS* cache, const char* key) {
    if (cache == NULL || key == NULL || DLIRS_STRICT_CHECK(cache)) {
        return false;
    }
    DLIRSEntry* value;
    if ((value = dqht_get(cache->lirs, key)) != NULL) {
        return value->in_cache;
    }
    return dqht_get(cache->q, key) != NULL;
}

bool dlirs_is_full(DLIRS* cache) {
    if (cache == NULL) {
        return false;
    }
    return (cache->hirs_count + cache->lirs_count) == cache->cache_size;
}

void dlirs_hit_lir(DLIRS* cache, const char* key) {
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
    if (dqht_insert(cache->lirs, key, value) != 0) {
        return;
    }
    if ((uintptr_t) lru_lir == (uintptr_t) value) {
        dlirs_prune(cache);
    }
}

// -1 = failure, 0 = in cache, 1 = not in cache
int dlirs_hir_in_lirs(DLIRS* cache, const char* key, DLIRSEntry** evicted) {
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
    if (dqht_remove(cache->lirs, key) == NULL
        || (hirsEntry = dqht_remove(cache->hirs, key)) == NULL) {
        dlirs_entry_destroy(entry);
        return -1;
    }
    dlirs_entry_destroy(hirsEntry);
    if (in_cache) {
        DLIRSEntry* qEntry;
        if ((qEntry = dqht_remove(cache->q, key)) == NULL) {
            return -1;
        }
        cache->hirs_count--;
        dlirs_entry_destroy(qEntry);
    } else {
        dlirs_adjust_size(cache, true);
        entry->in_cache = true;
        cache->non_resident--;
    }
    TRACE("Before eject lir loop");
    while (cache->lirs_count >= (size_t) cache->lirs_limit) {
        TRACE("Ejecting LIR: [Count: %zu] [Limit: %f]", cache->lirs_count, cache->lirs_limit);
        dlirs_eject_lir(cache);
    }
    TRACE("After eject lir loop");
    TRACE("Before eject hir loop");
    while ((cache->hirs_count + cache->lirs_count) >= (size_t) cache->cache_size) {
        TRACE("Ejecting HIR: [Count: %zu] [Limit: %zu]", (cache->hirs_count + cache->lirs_count), cache->cache_size);
        *evicted = dlirs_eject_hir(cache);
    }
    TRACE("After eject hir loop");
    dqht_insert(cache->lirs, key, entry);
    cache->lirs_count++;
    return !in_cache;
}

void dlirs_prune(DLIRS* cache) {
    if (cache == NULL || DLIRS_STRICT_CHECK(cache)) {
        return;
    }
    dqht_print_table("PRUNE LIRS", cache->lirs);
    dqht_print_table("PRUNE HIRS", cache->hirs);
    dqht_print_table("PRUNE Q", cache->q);
    DLIRSEntry* entry;
    DLIRSEntry* entry1;
    DLIRSEntry* entry2;
    while ((entry = dqht_get_front(cache->lirs)) != NULL) {
        TRACE("Prune LIR entry %p", entry);
        if (entry->is_LIR || (entry1 = dqht_remove(cache->lirs, entry->key)) == NULL) {
            break;
        } else if ((entry2 = dqht_remove(cache->hirs, entry->key)) == NULL) {
            dlirs_entry_destroy(entry1);
            break;
        }
        TRACE("Prune removed [LIR: %p] [HIR: %p]", entry1, entry2);
        if (!entry->in_cache) {
            cache->non_resident--;
        }
        dlirs_entry_destroy(entry1);
        dlirs_entry_destroy(entry2);
        TRACE("Destroyed hir-lir");
    }
    dqht_print_table("POST PRUNE LIRS", cache->lirs);
    dqht_print_table("POST PRUNE HIRS", cache->hirs);
    dqht_print_table("POST PRUNE Q", cache->q);
}

void dlirs_adjust_size(DLIRS* cache, bool hit_nonresident_hir) {
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
        cache->lirs_limit = cache->cache_size - cache->hirs_limit;
        return;
    }
    cache->lirs_limit = math_min(
        cache->cache_size - 1,
        cache->lirs_limit + math_max(
            1,
            (int)((((float) cache->non_resident) / ((float) cache->demoted)) + 0.5)
        )
    );
    cache->hirs_limit = cache->cache_size - cache->lirs_limit;
}

void dlirs_eject_lir(DLIRS* cache) {
    if (cache == NULL || DLIRS_STRICT_CHECK(cache)) {
        return;
    }
    DLIRSEntry* lru = dqht_pop_front(cache->lirs);
    if (lru == NULL) {
        return;
    }
    cache->lirs_count--;
    lru->is_LIR = false;
    lru->is_demoted = true;
    cache->demoted++;
    if (dqht_insert(cache->q, lru->key, dlirs_entry_copy(lru)) != 0) {
        return;
    }
    dlirs_entry_destroy(lru);
    cache->hirs_count++;
    TRACE("Incremented HIRS count");
    dlirs_prune(cache);
}

DLIRSEntry* dlirs_eject_hir(DLIRS* cache) {
    if (cache == NULL || DLIRS_STRICT_CHECK(cache)) {
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
    if (cache == NULL || key == NULL || DLIRS_STRICT_CHECK(cache)) {
        return;
    }
    DLIRSEntry* entry = dqht_get(cache->q, key);
    if (entry == NULL) {
        return;
    }
    if (entry->is_demoted) {
        dlirs_adjust_size(cache, false);
        entry->is_demoted = true;
        cache->demoted--;
    }
    if (dqht_insert(cache->q, key, entry) != 0
        || dqht_insert(cache->lirs, key, dlirs_entry_copy(entry)) != 0
        || dqht_insert(cache->hirs, key, dlirs_entry_copy(entry)) != 0) {
        return;
    }
    dlirs_limit_stack(cache);
}

void dlirs_limit_stack(DLIRS* cache) {
    if (cache == NULL || DLIRS_STRICT_CHECK(cache)) {
        return;
    }
    while ((cache->hirs_count + cache->lirs_count + cache->non_resident) > (2 * cache->cache_size)) {
        DLIRSEntry* lru = dqht_pop_front(cache->hirs);
        DLIRSEntry* entry;
        if (lru == NULL) {
            break;
        } else if ((entry = dqht_remove(cache->lirs, lru->key)) == NULL) {
            dlirs_entry_destroy(lru);
            break;
        }
        if (!lru->in_cache) {
            cache->non_resident--;
        }
        dlirs_entry_destroy(lru);
        dlirs_entry_destroy(entry);
    }
}

int dlirs_miss(DLIRS* cache, const char* key, void* value, DLIRSEntry** evicted) {
    if (cache == NULL || key == NULL || DLIRS_STRICT_CHECK(cache)) {
        return -1;
    }
    if (cache->lirs_count < cache->lirs_limit && cache->hirs_count == 0) {
        TRACE("MISS ENTRY: %s", key);
        DLIRSEntry* entry = dlirs_entry_create(key, value);
        if (entry == NULL) {
            return -1;
        }
        entry->is_LIR = true;
        if (dqht_insert(cache->lirs, key, entry) != 0) {
            return -1;
        }
        cache->lirs_count++;
        *evicted = NULL;
        return 0;
    }
    while ((cache->hirs_count + cache->lirs_count) >= cache->cache_size) {
        while (cache->lirs_count > cache->lirs_limit) {
            dlirs_eject_lir(cache);
        }
        *evicted = dlirs_eject_hir(cache);
    }
    TRACE("MISS ENTRY 2: %s", key);
    DLIRSEntry* entry = dlirs_entry_create(key, value);
    if (entry == NULL
        || dqht_insert(cache->q, key, entry) != 0
        || dqht_insert(cache->lirs, key, dlirs_entry_copy(entry)) != 0
        || dqht_insert(cache->hirs, key, dlirs_entry_copy(entry)) != 0) {
        return -1;
    }
    cache->hirs_count++;
    dlirs_limit_stack(cache);
    return 0;
}

// -1 = failure, 0 = miss, 1 = hit
int dlirs_request(DLIRS* cache, const char* key, void* value, DLIRSEntry** evicted) {
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
            dlirs_hit_lir(cache, key);
            TRACE("Hit LIR after");
        } else {
            TRACE("Hit HIR in LIRS before");
            miss = dlirs_hir_in_lirs(cache, key, evicted);
            TRACE("Hit HIR in LIRS after");
        }
    } else if (dqht_get(cache->q, key) != NULL) {
        TRACE("Hit HIR in Q before");
        dlirs_hit_hir_in_q(cache, key);
        TRACE("Hit HIR in Q after");
    } else {
        miss = 1;
        TRACE("Miss before");
        if (dlirs_miss(cache, key, value, evicted) != 0) {
            return -1;
        }
        TRACE("Miss after");
    }
    return !miss;
}

int dlirs_destroy(DLIRS* cache) {
    if (cache == NULL) {
        return 0;
    }
    dqht_destroy_handled(
        cache->q,
        (EntryValueDestroyHandler) lambda(void, (DLIRSEntry* entry, DLIRS* dlirs), {
            if (entry == NULL || entry->key == NULL) {
                return;
            } else if (dqht_get(dlirs->lirs, entry->key) == NULL
                       && dqht_get(dlirs->hirs, entry->key) == NULL) {
                dlirs_entry_destroy(entry);
            }
        }),
        cache
    );
    DEBUG("[Cache: %p, Table: %p] Destroyed Q table", cache, cache->q);
    dqht_destroy_handled(
        cache->hirs,
        (EntryValueDestroyHandler) lambda(void, (DLIRSEntry* entry, DLIRS* dlirs), {
            if (entry == NULL || entry->key == NULL) {
                return;
            } else if (dqht_get(dlirs->lirs, entry->key) == NULL) {
                dlirs_entry_destroy(entry);
            }
        }),
        cache
    );
    DEBUG("[Cache: %p, Table: %p] Destroyed HIRS table", cache, cache->hirs);
    dqht_destroy_handled(
        cache->lirs,
        (EntryValueDestroyHandler) lambda(void, (DLIRSEntry* entry, void* _ignored), {
            if (entry == NULL || entry->key == NULL) {
            return;
        }
            dlirs_entry_destroy(entry);
        }),
        NULL
    );
    DEBUG("[Cache: %p, Table: %p] Destroyed LIRS table", cache, cache->lirs);
    free(cache);
    return 0;
}
