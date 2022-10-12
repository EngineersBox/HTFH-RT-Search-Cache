#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_HASHING_H
#define HTFH_RT_SEARCH_CACHE_HASHING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <string.h>
#include "../../preprocessor/checks.h"
#include "../cache_key.h"

#ifdef ARCH_64_BIT
#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL
#else
#define FNV_OFFSET 2166136261UL
#define FNV_PRIME 16777619UL
#endif
#define MEIYAN_H 0x811c9dc5
#define MEIYAN_MULTIPLIER 0xad3e7

static size_t fnv1a_hash(const char* key) {
    size_t keySize = key_size(key);
    size_t hash = FNV_OFFSET;
    for (const char* p = key; (uintptr_t) p - (uintptr_t) key < keySize; p++) {
        hash ^= (size_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

static size_t meiyan_hash(const char *key) {
    int count = key_size(key);
    size_t h = MEIYAN_H;
    while (count >= 8) {
        h = (h ^ ((((*(uint32_t*)key) << 5) | ((*(uint32_t*)key) >> 27)) ^ *(uint32_t*)(key + 4))) * MEIYAN_MULTIPLIER;
        count -= 8;
        key += 8;
    }
#define tmp h = (h ^ *(uint16_t*)key) * MEIYAN_MULTIPLIER; key += 2;
    if (count & 4) { tmp tmp }
    if (count & 2) { tmp }
    if (count & 1) { h = (h ^ *key) * MEIYAN_MULTIPLIER; }
#undef tmp
    return h ^ (h >> 16);
}

#ifdef __cplusplus
};
#endif

#endif // HTFH_RT_SEARCH_CACHE_HASHING_H