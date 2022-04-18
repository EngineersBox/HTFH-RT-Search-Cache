#pragma once

#ifndef _C_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_CONTROLLER_
#define _C_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_CONTROLLER_

#ifdef __cplusplus
extern "C" {
#endif

#include "block.h"

/* The TLSF control structure. */
typedef struct Controller {
    /* Empty lists point at this block to indicate they are free. */
    BlockHeader block_null;

    /* Bitmaps for free lists. */
    unsigned int fl_bitmap;
    unsigned int sl_bitmap[FL_INDEX_COUNT];

    /* Head of free lists. */
    BlockHeader* blocks[FL_INDEX_COUNT][SL_INDEX_COUNT];
} Controller;

BlockHeader* controller_search_suitable_block(Controller* control, int* fli, int* sli);
/* Remove a free block from the free list.*/
int controller_remove_free_block(Controller* control, BlockHeader* block, int fl, int sl);
/* Insert a free block into the free block list. */
int controller_insert_free_block(Controller* control, BlockHeader* block, int fl, int sl);
/* Remove a given block from the free list. */
int controller_block_remove(Controller* control, BlockHeader* block);
/* Insert a given block into the free list. */
int controller_block_insert(Controller* control, BlockHeader* block);
/* Merge a just-freed block with an adjacent previous free block. */
BlockHeader* controller_block_merge_prev(Controller* control, BlockHeader* block);
/* Merge a just-freed block with an adjacent free block. */
BlockHeader* controller_block_merge_next(Controller* control, BlockHeader* block);
/* Trim any trailing block space off the end of a block, return to pool. */
int controller_block_trim_free(Controller* control, BlockHeader* block, size_t size);
/* Trim any trailing block space off the end of a used block, return to pool. */
int controller_block_trim_used(Controller* control, BlockHeader* block, size_t size);
BlockHeader* controller_block_trim_free_leading(Controller* control, BlockHeader* block, size_t size);
BlockHeader* controller_block_locate_free(Controller* control, size_t size);
void* controller_block_prepare_used(Controller* control, BlockHeader* block, size_t size);
/* Clear structure and point all empty lists at the null block. */
int controller_new(Controller* control);

#ifdef __cplusplus
};
#endif

#endif // _C_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_CONTROLLER_