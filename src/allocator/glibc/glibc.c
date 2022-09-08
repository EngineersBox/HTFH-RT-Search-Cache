#include "glibc.h"
#include <stddef.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "../error/allocator_errno.h"
#include "../../preprocessor/checks.h"

#define gca_round_up(x, align) (((x) + (align) - 1) & -(align))
#define gca_as_bytes(x) ((char *)(x))
#define gca_as_header(x) ((Header*)(x))
#define gca_data_to_header(d) (gca_as_header(gca_as_bytes(d) - GCA_INFO_SIZE))
#define gca_header_to_data(h) (gca_as_bytes(h) + GCA_INFO_SIZE)
#define gca_info(w, h) (h->w##_info)
#define gca_size(w, h) (gca_info(w, h) & GCA_MASK_SIZE)
#define gca_used(w, h) (gca_info(w, h) & GCA_FLAG_USED)
#define gca_set_size(w, h, v) (gca_info(w, h) = (gca_info(w, h) & ~GCA_MASK_SIZE) | (v))
#define gca_set_used(w, h, v) (gca_info(w, h) = (gca_info(w, h) & ~GCA_FLAG_USED) | (v))
#define gca_is_sentinel(w, h) (gca_size(w, h) == 0)
#define gca_next(h) (gca_as_header(gca_header_to_data(h) + gca_size(curr, h)))
#define gca_prev(h) (gca_data_to_header(gca_as_bytes(h) - gca_size(prev, h)))

GlibcAllocator* gca_create(size_t heapSize) {
    GlibcAllocator* alloc = malloc(sizeof(*alloc));
    if (alloc == NULL) {
        set_alloc_errno(NULL_ALLOCATOR_INSTANCE);
        return NULL;
    }
    init_check(int, lock_result, htfh_lock_init(&alloc->mutex, PTHREAD_MUTEX_RECURSIVE), != 0) {
        set_alloc_errno_msg(MUTEX_LOCK_INIT, strerror(lock_result));
        return NULL;
    } else if (htfh_lock_lock_handled(&alloc->mutex) != 0) {
        return NULL;
    }
    alloc->heap = NULL;
    alloc->free_list = NULL;
    alloc->heap_size = heapSize;
    alloc->current_brk = alloc->heap = mmap(
        NULL,
        heapSize,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );
    if (alloc->heap == NULL) {
        set_alloc_errno(HEAP_MMAP_FAILED);
        htfh_lock_unlock_handled(&alloc->mutex);
        return NULL;
    }
    void* orig_brk;
    void* new_brk;
    if (!gca_handled_sbrk(alloc, GCA_SBRK_ALIGN, &orig_brk, &new_brk)) {
        set_alloc_errno(HEAP_MMAP_FAILED);
        htfh_lock_unlock_handled(&alloc->mutex);
        return NULL;
    }
    Header* bottom = gca_as_header(orig_brk);
    gca_set_size(prev, bottom, 0);
    gca_set_used(prev, bottom, 1);
    Header* top = gca_data_to_header(new_brk);
    gca_set_size(curr, top, 0);
    gca_set_used(curr, top, 1);
    gca_set_size(curr, bottom, GCA_SBRK_ALIGN - 2 * GCA_INFO_SIZE);
    gca_set_used(curr, bottom, 0);
    gca_set_size(prev, top, GCA_SBRK_ALIGN - 2 * GCA_INFO_SIZE);
    gca_set_used(prev, top, 0);
    gca_add_free_list(alloc, bottom);
    return htfh_lock_unlock_handled(&alloc->mutex) == 0 ? alloc : NULL;
}

int gca_destroy(GlibcAllocator* alloc) {
    if (alloc == NULL) {
        set_alloc_errno(BAD_DEALLOC);
        return -1;
    } else if (htfh_lock_lock_handled(&alloc->mutex) == -1) {
        return -1;
    } else if (alloc->heap != NULL && munmap(alloc->heap, alloc->heap_size) != 0) {
        set_alloc_errno(HEAP_UNMAP_FAILED);
        htfh_lock_unlock_handled(&alloc->mutex);
        return -1;
    } else if (htfh_lock_unlock_handled(&alloc->mutex) == -1) {
        return -1;
    }
    free(alloc);
    return 0;
}

void gca_add_free_list(GlibcAllocator* alloc, Header* header) {
    header->prev_free = NULL;
    header->next_free = alloc->free_list;
    if (alloc->free_list != NULL) {
        alloc->free_list->prev_free = header;
    }
    alloc->free_list = header;
}

void gca_remove_free_list(GlibcAllocator* alloc, Header* header) {
    Header* prev_free = header->prev_free;
    Header* next_free = header->next_free;
    if (alloc->free_list == header) {
        alloc->free_list = next_free;
    }
    if (prev_free != NULL) {
        prev_free->next_free = next_free;
    }
    if (next_free != NULL) {
        next_free->prev_free = prev_free;
    }
}

int gca_brk(GlibcAllocator* alloc, void* addr) {
    if (addr > (alloc->heap + (uintptr_t) alloc->heap_size)) {
        return -1;
    }
    alloc->current_brk = addr;
    return 0;
}

void* gca_sbrk(GlibcAllocator* alloc, intptr_t increment) {
    if (alloc->current_brk == NULL && gca_brk(alloc, 0) < 0) {
        return (void*) -1;
    } else if (increment == 0) {
        return alloc->current_brk;
    }
    void* oldbrk = alloc->current_brk;
    if (increment > 0
        ? ((uintptr_t) oldbrk + (uintptr_t) increment < (uintptr_t) oldbrk)
        : ((uintptr_t) oldbrk < (uintptr_t) -increment)){
        return (void*) -1;
    } else if (gca_brk(alloc, oldbrk + increment) < 0) {
        return (void*) -1;
    }
    return oldbrk;
}

bool gca_handled_sbrk(GlibcAllocator* alloc, size_t delta, void** orig_brk, void** new_brk) {
    if ((size_t)alloc->current_brk + delta < (size_t)alloc->current_brk) {
        return false;
    }
    void *last_brk;
    if ((last_brk = gca_sbrk(alloc, delta)) == (void *)-1) {
        return false;
    }
    alloc->current_brk = (void *)((char *)last_brk + delta);
    *orig_brk = last_brk;
    *new_brk = alloc->current_brk;
    return true;
}

bool gca_coalesce_next(GlibcAllocator* alloc, Header *header) {
    Header* next_adj = gca_next(header);
    if (gca_used(curr, next_adj)) {
        return false;
    }
    gca_remove_free_list(alloc, next_adj);
    size_t new_size = gca_size(curr, header) + GCA_INFO_SIZE + gca_size(curr, next_adj);
    Header* next_next_adj = gca_next(next_adj);
    gca_set_used(prev, next_next_adj, gca_used(curr, header));
    gca_set_size(prev, next_next_adj, new_size);
    gca_set_size(curr, header, new_size);
    return true;
}

Header* gca_coalesce_prev(GlibcAllocator* alloc, Header* header) {
    if (!gca_used(prev, header)) {
        header = gca_prev(header);
        gca_coalesce_next(alloc, header);
    }
    return header;
}

Header* gca_coalesce(GlibcAllocator* alloc, Header* header) {
    gca_coalesce_next(alloc, header);
    return gca_coalesce_prev(alloc, header);
}

Header* gca_find_free_block(GlibcAllocator* alloc, size_t aligned_size) {
    Header* header = alloc->free_list;
    while (header != NULL) {
        if (gca_size(curr, header) >= aligned_size) {
            return header;
        }
        header = header->next_free;
    }
    return NULL;
}

Header* gca_sbrk_new_block(GlibcAllocator* alloc, size_t aligned_size) {
    size_t page_size = gca_round_up(aligned_size + GCA_INFO_SIZE, GCA_SBRK_ALIGN);
    void *orig_brk, *new_brk;
    if (!gca_handled_sbrk(alloc, page_size, &orig_brk, &new_brk)) {
        return NULL;
    }
    Header* header = gca_data_to_header(orig_brk);
    gca_set_size(curr, header, page_size - GCA_INFO_SIZE);
    gca_set_used(curr, header, 0);
    Header* sentinel = gca_data_to_header(new_brk);
    gca_set_size(prev, sentinel, page_size - GCA_INFO_SIZE);
    gca_set_used(prev, sentinel, 0);
    gca_set_size(curr, sentinel, 0);
    gca_set_used(curr, sentinel, 1);
    gca_add_free_list(alloc, header);
    return gca_coalesce_prev(alloc, header);
}

Header* gca_split_block(GlibcAllocator* alloc, Header* header, size_t aligned_size) {
    size_t curr_size = gca_size(curr, header);
    if (curr_size < aligned_size + GCA_INFO_SIZE + GCA_DATA_ALIGN) {
        return NULL;
    }
    size_t split_size = curr_size - aligned_size - GCA_INFO_SIZE;
    Header* next_header = gca_next(header);
    gca_set_size(prev, next_header, split_size);
    gca_set_used(prev, next_header, 0);
    gca_set_size(curr, header, aligned_size);
    Header* split_header = gca_next(header);
    gca_set_size(prev, split_header, aligned_size);
    gca_set_used(prev, split_header, gca_used(curr, header));
    gca_set_size(curr, split_header, split_size);
    gca_set_used(curr, split_header, 0);
    gca_add_free_list(alloc, split_header);
    gca_coalesce_next(alloc, split_header);
    return split_header;
}

void* gca_malloc(GlibcAllocator* alloc, size_t size) {
    if (alloc == NULL || alloc->heap == NULL || size == 0) {
        set_alloc_errno(MALLOC_FAILED);
        return NULL;
    } else if (htfh_lock_lock_handled(&alloc->mutex) == -1) {
        return NULL;
    }
    size_t aligned_size = gca_round_up(size, GCA_DATA_ALIGN);
    if (aligned_size == 0) {
        set_alloc_errno(MALLOC_FAILED);
        htfh_lock_unlock_handled(&alloc->mutex);
        return NULL;
    }

    Header *header = gca_find_free_block(alloc, aligned_size);
    if (header == NULL) {
        header = gca_sbrk_new_block(alloc, aligned_size);
        if (header == NULL) {
            set_alloc_errno(MALLOC_FAILED);
            htfh_lock_unlock_handled(&alloc->mutex);
            return NULL;
        }
    }

    gca_split_block(alloc, header, aligned_size);
    gca_remove_free_list(alloc, header);
    gca_set_used(curr, header, 1);
    gca_set_used(prev, gca_next(header), 1);
    return htfh_lock_unlock_handled(&alloc->mutex) != 0 ? NULL : gca_header_to_data(header);
}

int gca_free(GlibcAllocator* alloc, void* ptr) {
    if (alloc == NULL || alloc->heap == NULL || ptr == NULL) {
        return -1;
    } else if (htfh_lock_lock_handled(&alloc->mutex) == -1) {
        return -1;
    }
    Header* header = gca_data_to_header(ptr);
    gca_set_used(curr, header, 0);
    gca_set_used(prev, gca_next(header), 0);
    gca_add_free_list(alloc, header);
    gca_coalesce(alloc, header);
    return htfh_lock_unlock_handled(&alloc->mutex);
}

void* gca_calloc(GlibcAllocator* alloc, size_t num, size_t size) {
    if (alloc == NULL || alloc->heap == NULL || size == 0 || num == 0 || num > SIZE_MAX / size) {
        return NULL;
    } else if (htfh_lock_lock_handled(&alloc->mutex) == -1) {
        return NULL;
    }
    void* ptr = gca_malloc(alloc, num * size);
    if (ptr != NULL) {
        memset(ptr, 0, num * size);
    }
    return htfh_lock_unlock_handled(&alloc->mutex) != 0 ? NULL : ptr;
}

void* gca_realloc(GlibcAllocator* alloc, void *ptr, size_t size) {
    if (alloc == NULL || alloc->heap == NULL || size == 0) {
        return NULL;
    } else if (htfh_lock_lock_handled(&alloc->mutex) == -1) {
        return NULL;
    } if (ptr == NULL) {
        void* new_ptr = gca_malloc(alloc, size);
        return htfh_lock_unlock_handled(&alloc->mutex) != 0 ? NULL : new_ptr;
    }
    size_t aligned_size = gca_round_up(size, GCA_DATA_ALIGN);
    Header* header = gca_data_to_header(ptr);
    size_t orig_size = gca_size(curr, header);
    if (aligned_size <= orig_size) {
        gca_split_block(alloc, header, aligned_size);
        return htfh_lock_unlock_handled(&alloc->mutex) != 0 ? NULL : ptr;
    } else if (gca_coalesce_next(alloc, header) && aligned_size <= gca_size(curr, header)) {
        gca_split_block(alloc, header, aligned_size);
        return htfh_lock_unlock_handled(&alloc->mutex) != 0 ? NULL : ptr;
    } else if (gca_is_sentinel(curr, gca_next(header))) {
        Header* next_alloc = gca_sbrk_new_block(alloc, aligned_size - gca_size(curr, header));
        if (next_alloc != NULL) {
            gca_coalesce_next(alloc, header);
            gca_split_block(alloc, header, aligned_size);
            return htfh_lock_unlock_handled(&alloc->mutex) != 0 ? NULL : ptr;
        }
    }
    void* new_ptr = gca_malloc(alloc, size);
    if (new_ptr != NULL) {
        memcpy(new_ptr, ptr, orig_size);
        gca_free(alloc, ptr);
    }
    return htfh_lock_unlock_handled(&alloc->mutex) != 0 ? NULL : new_ptr;
}