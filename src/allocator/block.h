#pragma once

#ifndef _C_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_BLOCK_
#define _C_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_BLOCK_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "constants.h"
#include "utils.h"
/*
** Block header structure.
**
** There are several implementation subtleties involved:
** - The prev_phys_block field is only valid if the previous block is free.
** - The prev_phys_block field is actually stored at the end of the
**   previous block. It appears at the beginning of this structure only to
**   simplify the implementation.
** - The next_free / prev_free fields are only valid if the block is free.
*/
typedef struct BlockHeader {
    /* Points to the previous physical block. */
    struct BlockHeader* prev_phys_block;

    /* The size of this block, excluding the block header. */
    size_t size;

    /* Next and previous free blocks. */
    struct BlockHeader* next_free;
    struct BlockHeader* prev_free;
} BlockHeader;

/*
** Since block sizes are always at least a multiple of 4, the two least
** significant bits of the size field are used to store the block status:
** - bit 0: whether block is busy or free
** - bit 1: whether previous block is busy or free
*/
static const size_t block_header_free_bit = 1 << 0;
static const size_t block_header_prev_free_bit = 1 << 1;

/*
** The size of the block header exposed to used blocks is the size field.
** The prev_phys_block field is stored *inside* the previous free block.
*/
static const size_t block_header_overhead = sizeof(size_t);

/* User data starts directly after the size field in a used block. */
static const size_t block_start_offset = offsetof(BlockHeader, size) + sizeof(size_t);

/*
** A free block must be large enough to store its header minus the size of
** the prev_phys_block field, and no larger than the number of addressable
** bits for FL_INDEX.
*/
static const size_t block_size_min = sizeof(BlockHeader) - sizeof(BlockHeader*);
static const size_t block_size_max = (size_t) 1 << FL_INDEX_MAX;

size_t block_size(const BlockHeader* block);
void block_set_size(BlockHeader* block, size_t size);
int block_is_last(const BlockHeader* block);
int block_is_free(const BlockHeader* block);
void block_set_free(BlockHeader* block);
void block_set_used(BlockHeader* block);
int block_is_prev_free(const BlockHeader* block);
void block_set_prev_free(BlockHeader* block);
void block_set_prev_used(BlockHeader* block);
BlockHeader* block_from_ptr(const void* ptr);
void* block_to_ptr(const BlockHeader* block);
/* Return location of next block after block of given size. */
BlockHeader* offset_to_block(const void* ptr, size_t size);
/* Return location of previous block. */
BlockHeader* block_prev(const BlockHeader* block);
/* Return location of next existing block. */
BlockHeader* block_next(const BlockHeader* block);
/* Link a new block with its physical neighbor, return the neighbor. */
BlockHeader* block_link_next(BlockHeader* block);
int block_mark_as_free(BlockHeader* block);
int block_mark_as_used(BlockHeader* block);
int block_can_split(BlockHeader* block, size_t size);
/* Split a block into two, the second of which is free. */
BlockHeader* block_split(BlockHeader* block, size_t size);
/* Absorb a free block's storage into an adjacent previous free block. */
BlockHeader* block_absorb(BlockHeader* prev, BlockHeader* block);

#ifdef __cplusplus
};
#endif

#endif // _C_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_BLOCK_