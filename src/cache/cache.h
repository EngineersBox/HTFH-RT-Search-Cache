#pragma once

#ifndef _C_HTFH_RT_SEARCH_CACHE_
#define _C_HTFH_RT_SEARCH_CACHE_

#ifdef __cplusplus
extern "C" {
#endif

#include "../allocator/htfh/htfh.h"
#include "dlirs/dlirs.h"
#include "../allocator/thread/lock.h"

typedef struct Cache {
    __htfh_rwlock_t rwlock;
    DLIRS* dlirs;
    Allocator* alloc;
} Cache;

/* Note: Use hirs_ratio = 0.01f */
Cache* cache_new(size_t heap_size, size_t ht_size, size_t cache_size, float hirs_ratio);
int cache_destroy(Cache* cache);

#ifdef __cplusplus
};
#endif

#endif // _C_HTFH_RT_SEARCH_CACHE_