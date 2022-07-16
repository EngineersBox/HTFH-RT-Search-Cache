#pragma once

#ifndef _C_HTFH_RT_SEARCH_CACHE_
#define _C_HTFH_RT_SEARCH_CACHE_

#ifdef __cplusplus
extern "C" {
#endif

#include "../allocator/htfh/htfh.h"
#include "dlirs/dlirs.h"
#include "../allocator/thread/lock.h"
#include "../allocator/alloc_manager.h"

typedef struct Cache {
    __htfh_rwlock_t rwlock;
    DLIRS* dlirs;
    Allocator* alloc;
} Cache;

/* Note: Use hirs_ratio = 0.01f */
Cache* cache_new(size_t heap_size, size_t ht_size, size_t cache_size, float hirs_ratio);
int cache_destroy(Cache* cache);

bool cache_contains(Cache* cache, const char* key);
bool cache_is_full(Cache* cache);

int cache_request(Cache* cache, const char* key, void* value, DLIRSEntry** evicted);

#ifdef __cplusplus
};
#endif

#endif // _C_HTFH_RT_SEARCH_CACHE_