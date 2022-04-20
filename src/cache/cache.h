#pragma once

#ifndef _C_HTFH_RT_SEARCH_CACHE_
#define _C_HTFH_RT_SEARCH_CACHE_

#ifdef __cplusplus
extern "C" {
#endif

#include "../htfh/htfh.h"
#include "../thread/lock.h"

typedef struct Cache {
    __htfh_rwlock_t rwlock;
    Allocator* alloc;
} Cache;

void* cache_new(size_t heap_size);
int cache_destroy(Cache* cache);

#ifdef __cplusplus
};
#endif

#endif // _C_HTFH_RT_SEARCH_CACHE_