#pragma once

#ifndef C_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_UTILS
#define C_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_UTILS

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "../error/allocator_errno.h"
#include <limits.h>
#include "constants.h"

/*
** Architecture-specific bit manipulation routines.
**
** TLSF achieves O(1) cost for malloc and free operations by limiting
** the search for a free block to a free list of guaranteed size
** adequate to fulfill the request, combined with efficient free list
** queries using bitmasks and architecture-specific bit-manipulation
** routines.
**
** Most modern processors provide instructions to count leading zeroes
** in a word, find the lowest and highest set bit, etc. These
** specific implementations will be used when available, falling back
** to a reasonably efficient generic implementation.
**
** NOTE: TLSF spec relies on ffs/fls returning value 0..31.
** ffs/fls return 1-32 by default, returning 0 for error.
*/

/*
** Detect whether or not we are building for a 32- or 64-bit (LP/LLP)
** architecture. There is no reliable portable method at compile-time.
*/
#if defined(__alpha__) || defined(__ia64__) || defined(__x86_64__) \
	|| defined(_WIN64) || defined(__LP64__) || defined(__LLP64__)
#define ARCH_64_BIT
#endif

/*
** gcc 3.4 and above have builtin support, specialized for architecture.
** Some compilers masquerade as gcc; patchlevel test filters them out.
*/
#if defined(__GNUC__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)) && defined(__GNUC_PATCHLEVEL__)
__attribute__((always_inline)) inline int htfh_ffs(unsigned int word) {
    return __builtin_ffs(word) - 1;
}

__attribute__((always_inline)) inline int htfh_fls(unsigned int word) {
    const int bit = word ? 32 - __builtin_clz(word) : 0;
    return bit - 1;
}
#else
/* Fall back to generic implementation. */
__attribute__((always_inline)) inline int htfh_fls_generic(unsigned int word) {
    int bit = 32;
    if (!word) bit -= 1;
    if (!(word & 0xffff0000)) { word <<= 16; bit -= 16; }
    if (!(word & 0xff000000)) { word <<= 8; bit -= 8; }
    if (!(word & 0xf0000000)) { word <<= 4; bit -= 4; }
    if (!(word & 0xc0000000)) { word <<= 2; bit -= 2; }
    if (!(word & 0x80000000)) { word <<= 1; bit -= 1; }
    return bit;
}

/* Implement ffs in terms of fls. */
__attribute__((always_inline)) inline int htfh_ffs(unsigned int word) {
    return htfh_fls_generic(word & (~word + 1)) - 1;
}

__attribute__((always_inline)) inline int htfh_fls(unsigned int word) {
    return htfh_fls_generic(word) - 1;
}

#endif

/* Possibly 64-bit version of htfh_fls. */
#if defined (ARCH_64_BIT)
__attribute__((always_inline)) inline int htfh_fls_sizet(size_t size) {
    int high = (int)(size >> 32);
    return high ? 32 + htfh_fls(high) : htfh_fls((int)size & 0xffffffff);
}
#else
#define htfh_fls_sizet htfh_fls
#endif

/*
** Cast and min/max macros and prevent double evaluation
*/

#define htfh_min(a,b) ({ \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b; \
})
#define htfh_max(a,b) ({ \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b; \
})

/*
** Set assert macro, if it has not been provided by the user.
*/
#include <assert.h>
#if !defined (htfh_assert)
#define htfh_assert assert
#endif

/*
** Static assertion mechanism.
*/

#define _htfh_glue2(x, y) x ## y
#define _htfh_glue(x, y) _htfh_glue2(x, y)
#define htfh_static_assert(exp) typedef char _htfh_glue(static_assert, __LINE__) [(exp) ? 1 : -1]

/* This code has been tested on 32- and 64-bit (LP/LLP) architectures. */
htfh_static_assert(sizeof(int) * CHAR_BIT == 32);
htfh_static_assert(sizeof(size_t) * CHAR_BIT >= 32);
htfh_static_assert(sizeof(size_t) * CHAR_BIT <= 64);

/* SL_INDEX_COUNT must be <= number of bits in sl_bitmap's storage type. */
htfh_static_assert(sizeof(unsigned int) * CHAR_BIT >= SL_INDEX_COUNT);

/* Ensure we've properly tuned our sizes. */
htfh_static_assert(ALIGN_SIZE == SMALL_BLOCK_SIZE / SL_INDEX_COUNT);

size_t align_up(size_t x, size_t align);
size_t align_down(size_t x, size_t align);
void* align_ptr(const void* ptr, size_t align);
void mapping_insert(size_t size, int* fli, int* sli);
/* This version rounds up to the next block size (for allocations) */
void mapping_search(size_t size, int* fli, int* sli);

#ifdef __cplusplus
};
#endif

#endif // C_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_UTILS
