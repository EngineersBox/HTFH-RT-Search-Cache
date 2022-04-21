#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_HASHING_H
#define HTFH_RT_SEARCH_CACHE_HASHING_H

#ifdef __cplusplus
extern "C" {
#endif

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

#include <inttypes.h>

static uint64_t fnv1a_hash(const char* key) {
    uint64_t hash = FNV_OFFSET;
    for (const char* p = key; *p; p++) {
        hash ^= (uint64_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

#ifdef __cplusplus
};
#endif

#endif // HTFH_RT_SEARCH_CACHE_HASHING_H