#pragma once

#ifndef _H_C_FIXED_HEAP_ALLOCATOR_ALLOCATOR_
#define _H_C_FIXED_HEAP_ALLOCATOR_ALLOCATOR_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../thread/lock.h"
#include "../../allocator/alloc_manager.h"

#define GCA_INFO_SIZE (2 * sizeof(size_t))
#define GCA_DATA_ALIGN (2 * sizeof(size_t))
#define GCA_SBRK_ALIGN 4096
#define GCA_MASK_SIZE ~0x7
#define GCA_FLAG_USED 0x1

typedef struct Header {
    size_t prev_info;
    size_t curr_info;
    struct Header* prev_free;
    struct Header* next_free;
} Header;

typedef struct GlibcAllocator {
    htfh_lock_t mutex;
    Header* free_list;
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

__attribute__((hot)) int gca_free(GlibcAllocator* alloc, void* ptr);
__attribute__((hot, malloc, alloc_size(2)
#ifdef gnu_version_10
, malloc(gca_free, 2)
#endif
)) void* gca_malloc(GlibcAllocator* alloc, size_t bytes);
__attribute__((hot, malloc, alloc_size(2, 3)
#ifdef gnu_version_10
, malloc(gca_free, 2)
#endif
)) void* gca_calloc(GlibcAllocator* alloc, size_t count, size_t bytes);
__attribute__((hot, alloc_size(3))) void* gca_realloc(GlibcAllocator* alloc, void* ptr, size_t size);

bool gca_handled_sbrk(GlibcAllocator* alloc, size_t delta, void** orig_brk, void** new_brk);
void* gca_sbrk(GlibcAllocator* alloc, intptr_t increment);
int gca_brk(GlibcAllocator* alloc, void* addr);

void gca_add_free_list(GlibcAllocator* alloc, Header* header);
void gca_remove_free_list(GlibcAllocator* alloc, Header* header);

bool gca_coalesce_next(GlibcAllocator* alloc, Header *header);
Header* gca_coalesce_prev(GlibcAllocator* alloc, Header* header);
Header* gca_coalesce(GlibcAllocator* alloc, Header* header);

Header* gca_find_free_block(GlibcAllocator* alloc, size_t aligned_size);
Header* gca_sbrk_new_block(GlibcAllocator* alloc, size_t aligned_size);
Header* gca_split_block(GlibcAllocator* alloc, Header* header, size_t aligned_size);


#ifdef __cplusplus
};
#endif

#endif // _H_C_FIXED_HEAP_ALLOCATOR_ALLOCATOR_