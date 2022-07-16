#pragma once

#ifndef _C_HTFH_RT_SEARCH_CACHE_DLIRS_
#define _C_HTFH_RT_SEARCH_CACHE_DLIRS_

#ifdef __cplusplus
extern "C" {
#endif

#include "../../allocator/htfh/htfh.h"
#include "../dequeue_hashtable/dq_hashtable.h"
#include "../../allocator/thread/lock.h"
#include "dlirs_entry.h"

#include "../allocator/alloc_manager.h"

typedef struct DLIRS {
    size_t cache_size;

    float hirs_ratio;
    float hirs_limit;
    float lirs_limit;

    size_t hirs_count;
    size_t lirs_count;

    size_t demoted;
    size_t non_resident;

    DequeueHashTable* lirs;
    DequeueHashTable* hirs;
    DequeueHashTable* q;
} DLIRS;

#ifdef DLIRS_ENABLE_STRICT
#define DLIRS_STRICT_CHECK(cache) (cache->lirs == NULL || cache->hirs == NULL || cache->q == NULL)
#else
#define DLIRS_STRICT_CHECK(cache) false
#endif

typedef struct DLIRSOptions {
    float hirs_ratio;
} DLIRSOptions;

DLIRS* dlirs_create(AM_ALLOCATOR_PARAM size_t ht_size, size_t cache_size, DLIRSOptions* options);
int dlirs_destroy(AM_ALLOCATOR_PARAM DLIRS* cache);

bool dlirs_contains(DLIRS* cache, const char* key);
bool dlirs_is_full(DLIRS* cache);

void dlirs_hit_lir(AM_ALLOCATOR_PARAM DLIRS* cache, const char* key);
// -1 = failure, 0 = not in cache, 1 = in cache
int dlirs_hir_in_lirs(AM_ALLOCATOR_PARAM DLIRS* cache, const char* key, DLIRSEntry** evicted);
void dlirs_prune(AM_ALLOCATOR_PARAM DLIRS* cache);
void dlirs_adjust_size(DLIRS* cache, bool hit_nonresident_hir);
void dlirs_eject_lir(AM_ALLOCATOR_PARAM DLIRS* cache);
DLIRSEntry* dlirs_eject_hir(AM_ALLOCATOR_PARAM DLIRS* cache);
void dlirs_hit_hir_in_q(AM_ALLOCATOR_PARAM DLIRS* cache, const char* key);
void dlirs_limit_stack(AM_ALLOCATOR_PARAM DLIRS* cache);

int dlirs_miss(AM_ALLOCATOR_PARAM DLIRS* cache, const char* key, void* value, DLIRSEntry** evicted);
// -1 = failure, 0 = miss, 1 = hit
int dlirs_request(AM_ALLOCATOR_PARAM DLIRS* cache, const char* key, void* value, DLIRSEntry** evicted);

#ifdef __cplusplus
};
#endif

#endif // _C_HTFH_RT_SEARCH_CACHE_DLIRS_