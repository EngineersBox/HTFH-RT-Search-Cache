#pragma once


#ifndef HTFH_RT_SEARCH_CACHE_ATOMIC_UTILS_H
#define HTFH_RT_SEARCH_CACHE_ATOMIC_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdatomic.h>

typedef struct ptr_pair { void* p[2]; } ptr_pair;

inline void swap(_Atomic(ptr_pair)* ptrs) {
    ptr_pair actual = { 0 };
    ptr_pair future = { 0 };
    while (!atomic_compare_exchange_weak(ptrs, &actual, future)) {
        future.p[0] = actual.p[1];
        future.p[1] = actual.p[0];
    }
}

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_ATOMIC_UTILS_H
