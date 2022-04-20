#pragma once

#ifndef _C_HTFH_RT_SEARCH_CACHE_DLIRS_
#define _C_HTFH_RT_SEARCH_CACHE_DLIRS_

#ifdef __cplusplus
extern "C" {
#endif

#include "../htfh/htfh.h"
#include "../thread/lock.h"

typedef struct DLIRS {
    __htfh_rwlock_t rwlock;
    Allocator* alloc;
} DLIRS;

void* dlirs_new(size_t heap_size);
int dlirs_destroy(DLIRS* cache);

#ifdef __cplusplus
};
#endif

#endif // _C_HTFH_RT_SEARCH_CACHE_DLIRS_