#include "block.h"

inline size_t block_size(const BlockHeader* block) {
    return block->size & ~(block_header_free_bit | block_header_prev_free_bit);
}

inline void block_set_size(BlockHeader* block, size_t size) {
    const size_t old_size = block->size;
    block->size = size | (old_size & (block_header_free_bit | block_header_prev_free_bit));
}

inline int block_is_last(const BlockHeader* block) {
    return block_size(block) == 0;
}

inline int block_is_free(const BlockHeader* block) {
    return (int) (block->size & block_header_free_bit);
}

inline void block_set_free(BlockHeader* block) {
    block->size |= block_header_free_bit;
}

inline void block_set_used(BlockHeader* block) {
    block->size &= ~block_header_free_bit;
}

inline int block_is_prev_free(const BlockHeader* block) {
    return (int) (block->size & block_header_prev_free_bit);
}

inline void block_set_prev_free(BlockHeader* block) {
    block->size |= block_header_prev_free_bit;
}

inline void block_set_prev_used(BlockHeader* block) {
    block->size &= ~block_header_prev_free_bit;
}

inline BlockHeader* block_from_ptr(const void* ptr) {
    return (BlockHeader*)((unsigned char*) ptr - block_start_offset);
}

inline void* block_to_ptr(const BlockHeader* block) {
    return (void*) (((unsigned char*) block) + block_start_offset);
}

/* Return location of next block after block of given size. */
inline BlockHeader* offset_to_block(const void* ptr, size_t size) {
    return (BlockHeader*) (((ptrdiff_t) ptr) + size);
}

/* Return location of previous block. */
BlockHeader* block_prev(const BlockHeader* block) {
    if (!block_is_prev_free(block)) {
        set_alloc_errno(PREV_BLOCK_NOT_FREE);
        return NULL;
    }
    return block->prev_phys_block;
}

/* Return location of next existing block. */
BlockHeader* block_next(const BlockHeader* block) {
    BlockHeader* next = offset_to_block(
        block_to_ptr(block),
        block_size(block) - block_header_overhead
    );
    if (block_is_last(block)) {
        set_alloc_errno(BLOCK_IS_LAST);
        return NULL;
    }
    return next;
}

/* Link a new block with its physical neighbor, return the neighbor. */
BlockHeader* block_link_next(BlockHeader* block) {
    BlockHeader* next = block_next(block);
    if (next == NULL) {
        return NULL;
    }
    next->prev_phys_block = block;
    return next;
}

int block_mark_as_free(BlockHeader* block) {
    /* Link the block to the next block, first. */
    BlockHeader* next = block_link_next(block);
    if (next == NULL) {
        return -1;
    }
    block_set_prev_free(next);
    block_set_free(block);
}

int block_mark_as_used(BlockHeader* block) {
    BlockHeader* next = block_next(block);
    if (next == NULL) {
        return -1;
    }
    block_set_prev_used(next);
    block_set_used(block);
    return 0;
}

int block_can_split(BlockHeader* block, size_t size) {
    return block_size(block) >= sizeof(BlockHeader) + size;
}

/* Split a block into two, the second of which is free. */
BlockHeader* block_split(BlockHeader* block, size_t size) {
    /* Calculate the amount of space left in the remaining block. */
    BlockHeader* remaining = offset_to_block(block_to_ptr(block), size - block_header_overhead);
    const size_t remain_size = block_size(block) - (size + block_header_overhead);
    if (block_to_ptr(remaining) != align_ptr(block_to_ptr(remaining), ALIGN_SIZE)) {
        set_alloc_errno(BLOCK_NOT_ALIGNED);
        return NULL;
    } else if (block_size(block) != remain_size + size + block_header_overhead) {
        set_alloc_errno(BLOCK_SIZE_MISMATCH);
        return NULL;
    }
    block_set_size(remaining, remain_size);
    if (block_size(remaining) < block_size_min) {
        set_alloc_errno(INVALID_BLOCK_SPLIT_SIZE);
        return NULL;
    }
    block_set_size(block, size);
    block_mark_as_free(remaining);
    return remaining;
}

/* Absorb a free block's storage into an adjacent previous free block. */
BlockHeader* block_absorb(BlockHeader* prev, BlockHeader* block) {
    if (block_is_last(prev)) {
        set_alloc_errno(BLOCK_IS_LAST);
        return NULL;
    }
    /* Note: Leaves flags untouched. */
    prev->size += block_size(block) + block_header_overhead;
    block_link_next(prev);
    return prev;
}