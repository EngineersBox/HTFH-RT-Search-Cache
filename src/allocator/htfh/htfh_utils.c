#include "htfh_utils.h"

/* This code has been tested on 32- and 64-bit (LP/LLP) architectures. */
htfh_static_assert(sizeof(int) * CHAR_BIT == 32);
htfh_static_assert(sizeof(size_t) * CHAR_BIT >= 32);
htfh_static_assert(sizeof(size_t) * CHAR_BIT <= 64);

/* SL_INDEX_COUNT must be <= number of bits in sl_bitmap's storage type. */
htfh_static_assert(sizeof(unsigned int) * CHAR_BIT >= SL_INDEX_COUNT);

/* Ensure we've properly tuned our sizes. */
htfh_static_assert(ALIGN_SIZE == SMALL_BLOCK_SIZE / SL_INDEX_COUNT);

size_t align_up(size_t x, size_t align) {
    if ((align & (align -1)) != 0) {
        set_alloc_errno(ALIGN_POWER_OF_TWO);
        return 0;
    }
    return (x + (align - 1)) & ~(align - 1);
}

size_t align_down(size_t x, size_t align) {
    if ((align & (align -1)) != 0) {
        set_alloc_errno(ALIGN_POWER_OF_TWO);
        return 0;
    }
    return x - (x & (align - 1));
}

void* align_ptr(const void* ptr, size_t align) {
    const  ptrdiff_t aligned = ((ptrdiff_t)(ptr) + (align - 1)) & ~(align - 1);
    if ((align & (align -1)) != 0) {
        set_alloc_errno(ALIGN_POWER_OF_TWO);
        return 0;
    }
    return (void*) aligned;
}

void mapping_insert(size_t size, int* fli, int* sli) {
    if (size < SMALL_BLOCK_SIZE) {
        /* Store small blocks in first list. */
        *fli = 0;
        *sli = (int) size / (SMALL_BLOCK_SIZE / SL_INDEX_COUNT);
        return;
    }
    *fli = htfh_fls_sizet(size);
    *sli = (int) (size >> (*fli - SL_INDEX_COUNT_LOG2)) ^ (1 << SL_INDEX_COUNT_LOG2);
    *fli -= (FL_INDEX_SHIFT - 1);
}

/* This version rounds up to the next block size (for allocations) */
void mapping_search(size_t size, int* fli, int* sli) {
    if (size >= SMALL_BLOCK_SIZE) {
        size += (1 << (htfh_fls_sizet(size) - SL_INDEX_COUNT_LOG2)) - 1;
    }
    mapping_insert(size, fli, sli);
}