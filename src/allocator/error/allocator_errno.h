#pragma once

#ifndef H_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_ERRNO
#define H_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_ERRNO

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>

typedef enum allocator_error_num {
    NONE,
    NULL_ALLOCATOR_INSTANCE,
    HEAP_ALREADY_MAPPED,
    HEAP_MMAP_FAILED,
    HEAP_UNMAP_FAILED,
    BAD_DEALLOC,
    MALLOC_FAILED,
    HEAP_MISALIGNED,

    MUTEX_LOCK_INIT,
    MUTEX_LOCK_LOCK,
    MUTEX_LOCK_UNLOCK,
    MUTEX_LOCK_DESTROY,

    RWLOCK_LOCK_INIT,
    RWLOCK_WRLOCK_LOCK,
    RWLOCK_RDLOCK_LOCK,
    RWLOCK_LOCK_UNLOCK,
    RWLOCK_LOCK_DESTROY,

    PREV_BLOCK_FREE,
    BLOCK_IS_LAST,
    NEXT_BLOCK_NULL,
    PREV_BLOCK_NULL,
    PREV_BLOCK_NOT_FREE,
    BLOCK_IS_NULL,
    NON_ZERO_BLOCK_SIZE,
    BLOCK_NOT_FREE,
    BLOCK_NOT_ALIGNED,
    BLOCK_SIZE_MISMATCH,
    INVALID_BLOCK_SPLIT_SIZE,
    BLOCK_ALREADY_FREED,
    CANNOT_REMOVE_BLOCK,

    ALIGN_POWER_OF_TWO,

    NULL_CONTROLLER_INSTANCE,

    SECOND_LEVEL_BITMAP_NULL,
    FIRST_LEVEL_BITMAP_NULL,

    HEAP_FULL,

    POOL_MISALIGNED,
    INVALID_POOL_SIZE,

    FREE_NULL_PTR,
    PTR_NOT_TO_BLOCK_HEADER,
    MERGE_PREV_FAILED,
    MERGE_NEXT_FAILED,

    GAP_TOO_SMALL,
} AllocatorErrno;


#define MAX_PREFIX_LENGTH 1024
#define MAX_ERR_LINE_LENGTH 1024
#define MAX_ERR_STRING_LENGTH 2048

extern __thread int alloc_errno;
extern __thread char alloc_errno_location[MAX_ERR_LINE_LENGTH];
extern __thread char alloc_errno_msg[MAX_ERR_STRING_LENGTH];
extern __thread char alloc_errno_strerr[MAX_ERR_LINE_LENGTH];

extern void get_alloc_errmsg(AllocatorErrno err);

#define set_alloc_errno(err) alloc_errno = err; sprintf(alloc_errno_location, "%s(%s:%d)", __func__, __FILE__, __LINE__)
#define set_alloc_errno_msg(err, msg) sprintf(alloc_errno_strerr, ": [%s]:", msg); set_alloc_errno(err)

#define alloc_perror(prefix) ({ \
    char trunc_prefix[MAX_PREFIX_LENGTH]; \
    size_t str_len = strlen(prefix); \
    if (str_len > MAX_PREFIX_LENGTH) { \
        str_len = MAX_PREFIX_LENGTH; \
    } \
    strncpy(trunc_prefix, prefix, str_len); \
    trunc_prefix[str_len] = 0; \
    get_alloc_errmsg(alloc_errno); \
    fprintf( \
        stderr, \
        "%s%s%s\n\tat %s(%s:%d)\n\tat %s\n", \
        trunc_prefix, \
        __alloc__errno_msg, \
        __alloc__errno_strerr, \
        __func__, __FILE__, __LINE__, \
        __alloc__errno_location \
    ); \
})

#ifdef __cplusplus
};
#endif

#endif // H_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_ERRNO