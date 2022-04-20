#include "htfh.h"
#include <stddef.h>
#include <sys/mman.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static size_t adjust_request_size(size_t size, size_t align) {
    if (!size) {
        return 0;
    }
    const size_t aligned = align_up(size, align);
    if (aligned < block_size_max) {
        return htfh_max(aligned, block_size_min);
    }
    return 0;
}

inline size_t htfh_size(void) {
    return sizeof(Controller);
}

inline size_t htfh_align_size(void) {
    return ALIGN_SIZE;
}

inline size_t htfh_block_size_min(void) {
    return block_size_min;
}

inline size_t htfh_block_size_max(void) {
    return block_size_max;
}

inline size_t htfh_pool_overhead(void) {
    return 2 * block_header_overhead;
}

inline size_t htfh_alloc_overhead(void) {
    return block_header_overhead;
}

void* htfh_add_pool(Allocator* alloc, void* mem, size_t bytes) {
    if (__htfh_lock_lock_handled(&alloc->mutex) == -1) {
        return NULL;
    }
    const size_t pool_overhead = htfh_pool_overhead();
    const size_t pool_bytes = align_down(bytes - pool_overhead, ALIGN_SIZE);
    if (((ptrdiff_t) mem % ALIGN_SIZE) != 0) {
        set_alloc_errno(POOL_MISALIGNED);
        __htfh_lock_unlock_handled(&alloc->mutex);
        return NULL;
    } else if (pool_bytes < block_size_min || pool_bytes > block_size_max) {
        char msg[100];
        sprintf(
            msg,
            "Memory pool must be between 0x%x and 0x%x00 bytes: ",
#ifdef ARCH_64_BIT
            (unsigned int)(pool_overhead + block_size_min),
            (unsigned int)((pool_overhead + block_size_max) / 256)
#else
            (unsigned int)(pool_overhead + block_size_min),
            (unsigned int)(pool_overhead + block_size_max)
#endif
        );
        set_alloc_errno_msg(INVALID_POOL_SIZE, msg);
        __htfh_lock_unlock_handled(&alloc->mutex);
        return NULL;
    }
    BlockHeader* block = offset_to_block(mem, -(ptrdiff_t) block_header_overhead);
    block_set_size(block, pool_bytes);
    block_set_free(block);
    block_set_prev_used(block);
    if (controller_block_insert(alloc->controller, block) != 0) {
        __htfh_lock_unlock_handled(&alloc->mutex);
        return NULL;
    }

    BlockHeader* next = block_link_next(block);
    block_set_size(next, 0);
    block_set_used(next);
    block_set_prev_free(next);

    return __htfh_lock_unlock_handled(&alloc->mutex) == 0 ? mem : NULL;
}

#if _DEBUG
int test_ffs_fls() {
	/* Verify ffs/fls work properly. */
	int rv = 0;
	rv += (htfh_ffs(0) == -1) ? 0 : 0x1;
	rv += (htfh_fls(0) == -1) ? 0 : 0x2;
	rv += (htfh_ffs(1) == 0) ? 0 : 0x4;
	rv += (htfh_fls(1) == 0) ? 0 : 0x8;
	rv += (htfh_ffs(0x80000000) == 31) ? 0 : 0x10;
	rv += (htfh_ffs(0x80008000) == 15) ? 0 : 0x20;
	rv += (htfh_fls(0x80000008) == 31) ? 0 : 0x40;
	rv += (htfh_fls(0x7FFFFFFF) == 30) ? 0 : 0x80;
#if defined (ARCH_64_BIT)
	rv += (htfh_fls_sizet(0x80000000) == 31) ? 0 : 0x100;
	rv += (htfh_fls_sizet(0x100000000) == 32) ? 0 : 0x200;
	rv += (htfh_fls_sizet(0xffffffffffffffff) == 63) ? 0 : 0x400;
#endif
	if (rv) {
		printf("test_ffs_fls: %x ffs/fls tests failed.\n", rv);
	}
	return rv;
}
#endif

Allocator* htfh_create(size_t bytes) {
#if _DEBUG
    if (test_ffs_fls()) {
		return 0;
	}
#endif
    if ((bytes % ALIGN_SIZE) != 0) {
        char msg[100];
        sprintf(msg, "Memory must be aligned to %u bytes", (unsigned int) ALIGN_SIZE);
        set_alloc_errno_msg(HEAP_MISALIGNED, msg);
        return NULL;
    }
    Allocator* alloc = malloc(sizeof(*alloc));
    if (alloc == NULL) {
        set_alloc_errno(NULL_ALLOCATOR_INSTANCE);
        return NULL;
    }
    int lock_result;
    if ((lock_result = __htfh_lock_init(&alloc->mutex, PTHREAD_MUTEX_RECURSIVE)) != 0) {
        set_alloc_errno_msg(MUTEX_LOCK_INIT, strerror(lock_result));
        return NULL;
    } else if (__htfh_lock_lock_handled(&alloc->mutex) != 0) {
        return NULL;
    }
    alloc->heap_size = bytes;
    alloc->controller = alloc->heap = mmap(
        NULL,
        bytes,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );
    if (alloc->heap == NULL) {
        set_alloc_errno(HEAP_MMAP_FAILED);
        __htfh_lock_unlock_handled(&alloc->mutex);
        return NULL;
    } else if (controller_new(alloc->controller) != 0) {
        __htfh_lock_unlock_handled(&alloc->mutex);
        return NULL;
    }
    htfh_add_pool(alloc, (char*)alloc->heap + htfh_size(), bytes - htfh_size());
    return __htfh_lock_unlock_handled(&alloc->mutex) == 0 ? alloc : NULL;
}

int htfh_destroy(Allocator* alloc) {
    if (alloc == NULL) {
        set_alloc_errno(NULL_ALLOCATOR_INSTANCE);
        return -1;
    } else if (alloc->controller == NULL) {
        set_alloc_errno(NULL_ALLOCATOR_INSTANCE);
        return -1;
    } else if (__htfh_lock_lock_handled(&alloc->mutex) != 0) {
        return -1;
    }
    if (munmap(alloc->heap, alloc->heap_size) != 0) {
        set_alloc_errno(HEAP_UNMAP_FAILED);
        __htfh_lock_unlock_handled(&alloc->mutex);
        return -1;
    }
    if (__htfh_lock_unlock_handled(&alloc->mutex) != 0) {
        return -1;
    }
    free(alloc);
    return 0;
}

void* htfh_malloc(Allocator* alloc, size_t size) {
    if (alloc == NULL) {
        set_alloc_errno(NULL_ALLOCATOR_INSTANCE);
        return NULL;
    } else if (alloc->controller == NULL) {
        set_alloc_errno(NULL_ALLOCATOR_INSTANCE);
        return NULL;
    } else if (__htfh_lock_lock_handled(&alloc->mutex) == -1) {
        return NULL;
    }
    const size_t adjust = adjust_request_size(size, ALIGN_SIZE);
    BlockHeader* block = controller_block_find_free(alloc->controller, adjust);
    if (block == NULL) {
        __htfh_lock_unlock_handled(&alloc->mutex);
        return NULL;
    }
    void* ptr = controller_block_mark_used(alloc->controller, block, adjust);
    return __htfh_lock_unlock_handled(&alloc->mutex) == 0 ? ptr : NULL;
}

int htfh_free(Allocator* alloc, void* ptr) {
    if (alloc == NULL) {
        set_alloc_errno(NULL_ALLOCATOR_INSTANCE);
        return -1;
    } else if (alloc->controller == NULL) {
        set_alloc_errno(NULL_ALLOCATOR_INSTANCE);
        return -1;
    } else if (__htfh_lock_lock_handled(&alloc->mutex) == -1) {
        return -1;
    } else if (ptr == NULL) {
        /* Don't attempt to free a NULL pointer. */
        return __htfh_lock_unlock_handled(&alloc->mutex);
    }
    BlockHeader* block = block_from_ptr(ptr);
    if (block == NULL) {
        set_alloc_errno(BLOCK_IS_NULL);
        __htfh_lock_unlock_handled(&alloc->mutex);
        return -1;
    } else if (block_is_free(block)) {
        set_alloc_errno(BLOCK_ALREADY_FREED);
        __htfh_lock_unlock_handled(&alloc->mutex);
        return -1;
    }
    block_mark_as_free(block);
    if ((block = controller_block_merge_prev(alloc->controller, block)) == NULL) {
        __htfh_lock_unlock_handled(&alloc->mutex);
        return -1;
    } else if ((block = controller_block_merge_next(alloc->controller, block)) == NULL) {
        __htfh_lock_unlock_handled(&alloc->mutex);
        return -1;
    } else if (controller_block_insert(alloc->controller, block) != 0) {
        __htfh_lock_unlock_handled(&alloc->mutex);
        return -1;
    }
    return __htfh_lock_unlock_handled(&alloc->mutex);
}

void* htfh_calloc(Allocator* alloc, size_t count, size_t bytes) {
    void* ptr = htfh_malloc(alloc, count * bytes);
    if (ptr != NULL) {
        memset(ptr, 0, count * bytes);
    }
    return ptr;
}

void* htfh_memalign(Allocator* alloc, size_t align, size_t size) {
    if (alloc == NULL) {
        set_alloc_errno(NULL_ALLOCATOR_INSTANCE);
        return NULL;
    } else if (alloc->controller == NULL) {
        set_alloc_errno(NULL_ALLOCATOR_INSTANCE);
        return NULL;
    } else if (__htfh_lock_lock_handled(&alloc->mutex) == -1) {
        return NULL;
    }
    const size_t adjust = adjust_request_size(size, ALIGN_SIZE);

    /*
    ** We must allocate an additional minimum block size bytes so that if
    ** our free block will leave an alignment gap which is smaller, we can
    ** trim a leading free block and release it back to the pool. We must
    ** do this because the previous physical block is in use, therefore
    ** the prev_phys_block field is not valid, and we can't simply adjust
    ** the size of that block.
    */
    const size_t gap_minimum = sizeof(BlockHeader);
    const size_t size_with_gap = adjust_request_size(adjust + align + gap_minimum, align);

    /*
    ** If alignment is less than or equals base alignment, we're done.
    ** If we requested 0 bytes, return null, as htfh_malloc(0) does.
    */
    const size_t aligned_size = (adjust && align > ALIGN_SIZE) ? size_with_gap : adjust;

    BlockHeader* block = controller_block_find_free(alloc->controller, aligned_size);
    if (sizeof(BlockHeader) != block_size_min + block_header_overhead) {
        set_alloc_errno(BLOCK_SIZE_MISMATCH);
        __htfh_lock_unlock_handled(&alloc->mutex);
        return NULL;
    } else if (block != NULL) {
        void* ptr = block_to_ptr(block);
        void* aligned = align_ptr(ptr, align);
        size_t gap = (size_t)((ptrdiff_t) aligned - (ptrdiff_t) ptr);

        /* If gap size is too small, offset to next aligned boundary. */
        if (gap && gap < gap_minimum) {
            const size_t offset = htfh_max(gap_minimum - gap, align);
            const void* next_aligned = (void*)((ptrdiff_t) aligned + offset);
            aligned = align_ptr(next_aligned, align);
            gap = (size_t)((ptrdiff_t) aligned - (ptrdiff_t) ptr);
        }

        if (gap) {
            if (gap < gap_minimum) {
                set_alloc_errno(GAP_TOO_SMALL);
                __htfh_lock_unlock_handled(&alloc->mutex);
                return NULL;
            } else if ((block = controller_block_trim_free_leading(alloc->controller, block, gap)) == NULL) {
                __htfh_lock_unlock_handled(&alloc->mutex);
                return NULL;
            }
        }
    }

    void* ptr = controller_block_mark_used(alloc->controller, block, adjust);
    return ptr != NULL && __htfh_lock_unlock_handled(&alloc->mutex) == 0 ? ptr : NULL;
}

/*
** The TLSF block information provides us with enough information to
** provide a reasonably intelligent implementation of realloc, growing or
** shrinking the currently allocated block as required.
**
** This routine handles the somewhat esoteric edge cases of realloc:
** - a non-zero size with a null pointer will behave like malloc
** - a zero size with a non-null pointer will behave like free
** - a request that cannot be satisfied will leave the original buffer
**   untouched
** - an extended buffer size will leave the newly-allocated area with
**   contents undefined
*/
void* htfh_realloc(Allocator* alloc, void* ptr, size_t size) {
    if (alloc == NULL) {
        set_alloc_errno(NULL_ALLOCATOR_INSTANCE);
        return NULL;
    } else if (alloc->controller == NULL) {
        set_alloc_errno(NULL_ALLOCATOR_INSTANCE);
        return NULL;
    } else if (__htfh_lock_lock_handled(&alloc->mutex) == -1) {
        return NULL;
    }
    void* p = NULL;
    /* Zero-size requests are treated as free. */
    if (ptr && size == 0 && htfh_free(alloc, ptr) != 0) {
        __htfh_lock_unlock_handled(&alloc->mutex);
        return NULL;
    } else if (!ptr) {
        /* Requests with NULL pointers are treated as malloc. */
        p = htfh_malloc(alloc, size);
        return __htfh_lock_unlock_handled(&alloc->mutex) == 0 ? p : NULL;
    }
    BlockHeader* block = block_from_ptr(ptr);
    BlockHeader* next = block_next(block);

    const size_t cursize = block_size(block);
    const size_t combined = cursize + block_size(next) + block_header_overhead;
    const size_t adjust = adjust_request_size(size, ALIGN_SIZE);

    if (block_is_free(block)) {
        set_alloc_errno(BLOCK_ALREADY_FREED);
        __htfh_lock_unlock_handled(&alloc->mutex);
        return NULL;
    }

    /*
    ** If the next block is used, or when combined with the current
    ** block, does not offer enough space, we must reallocate and copy.
    */
    if (adjust > cursize && (!block_is_free(next) || adjust > combined)) {
        if ((p = htfh_malloc(alloc, size)) != NULL) {
            memcpy(p, ptr, htfh_min(cursize, size));
            if (htfh_free(alloc, ptr) != 0) {
                __htfh_lock_unlock_handled(&alloc->mutex);
                return NULL;
            }
        }
        return __htfh_lock_unlock_handled(&alloc->mutex) == 0 ? p : NULL;
    } else if (adjust > cursize) {
        /* Do we need to expand to the next block? */
        if (controller_block_merge_next(alloc->controller, block) != 0) {
            __htfh_lock_unlock_handled(&alloc->mutex);
            return NULL;
        }
        block_mark_as_used(block);
    }

    /* Trim the resulting block and return the original pointer. */
    if (controller_block_trim_used(alloc->controller, block, adjust) != 0) {
        __htfh_lock_unlock_handled(&alloc->mutex);
        return NULL;
    }
    return __htfh_lock_unlock_handled(&alloc->mutex) == 0 ? ptr : NULL;
}