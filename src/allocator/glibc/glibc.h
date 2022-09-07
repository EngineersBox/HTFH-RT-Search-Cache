#pragma once

#ifndef _H_C_FIXED_HEAP_ALLOCATOR_ALLOCATOR_
#define _H_C_FIXED_HEAP_ALLOCATOR_ALLOCATOR_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "../thread/lock.h"

typedef long Align;

typedef union Header {
    struct {
        union Header *ptr;
        unsigned size;
    } s;
    Align x;
} Header;

typedef enum AllocationMethod {
    BEST_FIT,
    FIRST_FIT,
    NEXT_FIT,
} AllocationMethod;

#ifndef NALLOC
#define NALLOC 1024
#endif

typedef struct GlibcAllocator {
    AllocationMethod method;
    htfh_lock_t mutex;
    Header base;
    Header* freep;
    void* current_brk;
    size_t heap_size;
    void* heap;
} GlibcAllocator;

GlibcAllocator* gca_create(size_t heapSize);
int gca_destroy(GlibcAllocator* alloc);

#if defined(__GNUC__) \
    && __GNUC__ >= 10    \
    && (__GNUC__ > 10 || (__GNUC__ >= 0 && __GNUC_MINOR__ >= 0)) \
    && defined(__GNUC_PATCHLEVEL__)
#define gnu_version_10
#endif

__attribute__((hot)) int gca_free(GlibcAllocator* alloc, void* ap);
__attribute__((hot, malloc, alloc_size(2)
#ifdef gnu_version_10
, malloc(htfh_free, 2)
#endif
)) void* gca_malloc(GlibcAllocator* alloc, unsigned nbytes);
__attribute__((hot, malloc, alloc_size(2, 3)
#ifdef gnu_version_10
, malloc(htfh_free, 2)
#endif
)) void* gca_calloc(GlibcAllocator* alloc, unsigned count, unsigned nbytes);

__attribute__((hot)) void* gca_sbrk(GlibcAllocator* alloc, intptr_t increment);
__attribute__((hot)) int gca_brk(GlibcAllocator* alloc, void* addr);

#ifdef __cplusplus
};
#endif

#endif // _H_C_FIXED_HEAP_ALLOCATOR_ALLOCATOR_