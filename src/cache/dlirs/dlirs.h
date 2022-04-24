#pragma once

#ifndef _C_HTFH_RT_SEARCH_CACHE_DLIRS_
#define _C_HTFH_RT_SEARCH_CACHE_DLIRS_

#ifdef __cplusplus
extern "C" {
#endif

#include "../../allocator/htfh/htfh.h"
#include "../dequeue_hashtable/dq_hashtable.h"
#include "../../allocator/thread/lock.h"

typedef struct DLIRS {
    __htfh_rwlock_t rwlock;
    size_t cache_size;
    size_t window_size;

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

    size_t time;

    Allocator* alloc;
} DLIRS;

void* dlirs_new(size_t heap_size, size_t ht_size, size_t cache_size, size_t window_size, float hirs_limit);
int dlirs_destroy(DLIRS* cache);

#ifdef __cplusplus
};
#endif

#endif // _C_HTFH_RT_SEARCH_CACHE_DLIRS_