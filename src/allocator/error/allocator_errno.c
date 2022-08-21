#include "allocator_errno.h"
#include <string.h>

_Thread_local int alloc_errno = NONE;
_Thread_local char alloc_errno_location[MAX_ERR_LINE_LENGTH];
_Thread_local char alloc_errno_msg[MAX_ERR_STRING_LENGTH];
_Thread_local char alloc_errno_strerr[MAX_ERR_LINE_LENGTH];

#define enum_error(enum_val, err_msg) case enum_val: strcpy(alloc_errno_msg, err_msg); break;

inline void get_alloc_errmsg(AllocatorErrno err) {
    switch (err) {
        enum_error(NULL_ALLOCATOR_INSTANCE, "Allocator is not initialised")
        enum_error(HEAP_ALREADY_MAPPED, "Managed heap has already been allocated")
        enum_error(HEAP_MMAP_FAILED, "Failed to map memory for heap")
        enum_error(HEAP_UNMAP_FAILED, "Failed to unmap anonymous memory for heap")
        enum_error(BAD_DEALLOC, "Unable to destruct Allocator instance")
        enum_error(MALLOC_FAILED, "Unable to reserve memory")
        enum_error(HEAP_MISALIGNED, "Heap size is not aligned correctly")
        enum_error(MUTEX_LOCK_INIT, "Creation of mutex lock failed")
        enum_error(MUTEX_LOCK_LOCK, "Unable to lock mutex")
        enum_error(MUTEX_LOCK_UNLOCK, "Unable to unlock mutex")
        enum_error(MUTEX_LOCK_DESTROY, "Failed to destroy mutex lock")
        enum_error(RWLOCK_LOCK_INIT, "Creation of rwlock failed")
        enum_error(RWLOCK_WRLOCK_LOCK, "Unable to write-lock rwlock")
        enum_error(RWLOCK_RDLOCK_LOCK, "Unable to read-lock rwlock")
        enum_error(RWLOCK_LOCK_UNLOCK, "Unable to unlock rwlock")
        enum_error(RWLOCK_LOCK_DESTROY, "Failed to destroy rwlock")
        enum_error(PREV_BLOCK_FREE, "Previous block must be free")
        enum_error(BLOCK_IS_LAST, "Current block is last, next not present")
        enum_error(NEXT_BLOCK_NULL, "Next block is null")
        enum_error(PREV_BLOCK_NULL, "Previous block is null")
        enum_error(PREV_BLOCK_NOT_FREE, "Previous block must be free")
        enum_error(BLOCK_IS_NULL, "Block in context is null")
        enum_error(NON_ZERO_BLOCK_SIZE, "Block size must be non-zero")
        enum_error(BLOCK_NOT_FREE, "Block in context is not free")
        enum_error(BLOCK_NOT_ALIGNED, "Block was not aligned correctly")
        enum_error(BLOCK_SIZE_MISMATCH, "Size of block in context did not match required structural size")
        enum_error(INVALID_BLOCK_SPLIT_SIZE, "Block was split with invalid size")
        enum_error(ALIGN_POWER_OF_TWO, "Must align to a power of two")
        enum_error(NULL_CONTROLLER_INSTANCE, "Controller is not initialised")
        enum_error(SECOND_LEVEL_BITMAP_NULL, "Second level bitmap is null")
        enum_error(FIRST_LEVEL_BITMAP_NULL, "First level bitmap is null")
        enum_error(HEAP_FULL, "Cannot allocate, heap is full")
        enum_error(POOL_MISALIGNED, "Memory pool was not aligned correctly")
        enum_error(INVALID_POOL_SIZE, "Memory pool was of an invalid size")
        enum_error(FREE_NULL_PTR, "Attempted to free null pointer")
        enum_error(PTR_NOT_TO_BLOCK_HEADER, "Pointer does not point to a block header")
        enum_error(BLOCK_ALREADY_FREED, "Block was already freed")
        enum_error(MERGE_PREV_FAILED, "Unable to merge free block with previous")
        enum_error(MERGE_NEXT_FAILED, "Unable to merge free block with next")
        enum_error(CANNOT_REMOVE_BLOCK, "Unable to remove block")
        enum_error(GAP_TOO_SMALL, "Gap size is too small")
        enum_error(NONE, "")
        default: break;
    }
}
