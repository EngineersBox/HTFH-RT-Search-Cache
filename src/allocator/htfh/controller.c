#include "controller.h"

#define largest_bit_class(index) (~0U << (index))

BlockHeader* controller_search_suitable_block(Controller* control, int* fli, int* sli) {
    unsigned int sl_map = control->sl_bitmap[*fli] & largest_bit_class(*sli);
    if (!sl_map) {
        /* No block exists. Search in the next largest first-level list. */
        const unsigned int fl_map = control->fl_bitmap & largest_bit_class((*fli) + 1);
        if (!fl_map) {
            /* No free blocks available, memory has been exhausted. */
            set_alloc_errno(HEAP_FULL);
            return NULL;
        }
        *fli = htfh_ffs(fl_map);
        sl_map = control->sl_bitmap[*fli];
    }
    if (!sl_map) {
        set_alloc_errno(SECOND_LEVEL_BITMAP_NULL);
        return NULL;
    }
    *sli = htfh_ffs(sl_map);
    /* Return the first block in the free list. */
    return control->blocks[*fli][*sli];
}

/* Remove a free block from the free list.*/
int controller_remove_free_block(Controller* control, BlockHeader* block, int fl, int sl) {
    BlockHeader* prev = block->prev_free;
    BlockHeader* next = block->next_free;
    if (prev == NULL) {
        set_alloc_errno(PREV_BLOCK_NULL);
        return -1;
    } else if (next == NULL) {
        set_alloc_errno(NEXT_BLOCK_NULL);
        return -1;
    }
    next->prev_free = prev;
    prev->next_free = next;

    if (control->blocks[fl][sl] != block) {
        return 0;
    }
    /* If this block is the head of the free list, set new head. */
    control->blocks[fl][sl] = next;
    if (next != &control->block_null) {
        return 0;
    }
    /* If the new head is null, clear the bitmap. */
    control->sl_bitmap[fl] &= ~(1U << sl);
    /* If the second bitmap is now empty, clear the fl bitmap. */
    if (!control->sl_bitmap[fl]) {
        control->fl_bitmap &= ~(1U << fl);
    }
    return 0;
}

/* Insert a free block into the free block list. */
int controller_insert_free_block(Controller* control, BlockHeader* block, int fl, int sl) {
    BlockHeader* current = control->blocks[fl][sl];
    if (current == NULL) {
        set_alloc_errno_msg(BLOCK_IS_NULL, "Free list cannot have null entry");
        return -1;
    } else if (block == NULL) {
        set_alloc_errno_msg(BLOCK_IS_NULL, "Cannot insert null entry into free list");
        return -1;
    }
    block->next_free = current;
    block->prev_free = &control->block_null;
    current->prev_free = block;
    if (block_to_ptr(block) != align_ptr(block_to_ptr(block), ALIGN_SIZE)) {
        set_alloc_errno(BLOCK_NOT_ALIGNED);
        return -1;
    }
    /*
    ** Insert the new block at the head of the list, and mark the first-
    ** and second-level bitmaps appropriately.
    */
    control->blocks[fl][sl] = block;
    control->fl_bitmap |= (1U << fl);
    control->sl_bitmap[fl] |= (1U << sl);
    return 0;
}

/* Remove a given block from the free list. */
int controller_block_remove(Controller* control, BlockHeader* block) {
    int fl, sl;
    mapping_insert(block_size(block), &fl, &sl);
    return controller_remove_free_block(control, block, fl, sl);
}

/* Insert a given block into the free list. */
int controller_block_insert(Controller* control, BlockHeader* block) {
    int fl, sl;
    mapping_insert(block_size(block), &fl, &sl);
    return controller_insert_free_block(control, block, fl, sl);
}

/* Merge a just-freed block with an adjacent previous free block. */
BlockHeader* controller_block_merge_prev(Controller* control, BlockHeader* block) {
    if (!block_is_prev_free(block)) {
        return block;
    }
    BlockHeader* prev = block_prev(block);
    if (prev == NULL) {
        set_alloc_errno(PREV_BLOCK_NULL);
        return NULL;
    } else if (!block_is_free(prev)) {
        set_alloc_errno(BLOCK_NOT_FREE);
        return NULL;
    } else if (controller_block_remove(control, prev) != 0) {
        return NULL;
    }
    block = block_absorb(prev, block);
    return block;
}

/* Merge a just-freed block with an adjacent free block. */
BlockHeader* controller_block_merge_next(Controller* control, BlockHeader* block) {
    BlockHeader* next = block_next(block);
    if (next == NULL) {
        return NULL;
    } else if (!block_is_free(next)) {
        return block;
    } else if (block_is_last(block)) {
        set_alloc_errno(BLOCK_IS_LAST);
        return NULL;
    } else if (controller_block_remove(control, next) != 0) {
        return NULL;
    }
    block = block_absorb(block, next);
    return block;
}

/* Trim any trailing block space off the end of a block, return to pool. */
int controller_block_trim_free(Controller* control, BlockHeader* block, size_t size) {
    if (!block_is_free(block)) {
        set_alloc_errno(BLOCK_NOT_FREE);
        return -1;
    } else if (!block_can_split(block, size)) {
        return 0;
    }
    BlockHeader* remaining_block = block_split(block, size);
    if (remaining_block == NULL) {
        return -1;
    }
    block_link_next(block);
    block_set_prev_free(remaining_block);
    return controller_block_insert(control, remaining_block);
}

/* Trim any trailing block space off the end of a used block, return to pool. */
int controller_block_trim_used(Controller* control, BlockHeader* block, size_t size) {
    if (!block_is_free(block)) {
        set_alloc_errno(BLOCK_NOT_FREE);
        return -1;
    } else if (!block_can_split(block, size)) {
        return 0;
    }
    /* If the next block is free, we must coalesce. */
    BlockHeader* remaining_block = block_split(block, size);
    if (remaining_block == NULL) {
        return -1;
    }
    block_set_prev_used(remaining_block);
    if ((remaining_block = controller_block_merge_next(control, remaining_block)) == NULL) {
        return -1;
    }
    return controller_block_insert(control, remaining_block);
}

BlockHeader* controller_block_trim_free_leading(Controller* control, BlockHeader* block, size_t size) {
    if (control == NULL) {
        set_alloc_errno(NULL_CONTROLLER_INSTANCE);
        return NULL;
    } else if (block == NULL) {
        set_alloc_errno(BLOCK_IS_NULL);
        return NULL;
    }
    if (!block_can_split(block, size)) {
        return block;
    }
    BlockHeader* remaining_block = block_split(block, size - block_header_overhead);
    block_set_prev_free(remaining_block);
    block_link_next(block);
    if (controller_block_insert(control, block) != 0) {
        return NULL;
    }
    return remaining_block;
}

BlockHeader* controller_block_find_free(Controller* control, size_t size) {
    if (control == NULL) {
        set_alloc_errno(NULL_CONTROLLER_INSTANCE);
        return NULL;
    } else if (!size) {
        return NULL;
    }
    int fl = 0;
    int sl = 0;
    mapping_search(size, &fl, &sl);
    if (fl >= FL_INDEX_COUNT) {
        return NULL;
    }
    BlockHeader* block = NULL;
    if ((block = controller_search_suitable_block(control, &fl, &sl)) == NULL) {
        return block;
    } else if (block_size(block) < size) {
        set_alloc_errno(BLOCK_SIZE_MISMATCH);
        return NULL;
    }
    return controller_remove_free_block(control, block, fl, sl) == 0 ? block : NULL;
}

void* controller_block_mark_used(Controller* control, BlockHeader* block, size_t size) {
    if (control == NULL) {
        set_alloc_errno(NULL_CONTROLLER_INSTANCE);
        return NULL;
    } else if (block == NULL) {
        set_alloc_errno(BLOCK_IS_NULL);
        return NULL;
    } else if (size <= 0) {
        set_alloc_errno(NON_ZERO_BLOCK_SIZE);
        return NULL;
    } else if (controller_block_trim_free(control, block, size) != 0) {
        return NULL;
    }
    return block_mark_as_used(block) == 0 ? block_to_ptr(block) : NULL;
}

/* Clear structure and point all empty lists at the null block. */
int controller_new(Controller* control) {
    if (control == NULL) {
        set_alloc_errno(NULL_CONTROLLER_INSTANCE);
        return -1;
    }
    control->block_null.prev_free = control->block_null.next_free = &control->block_null;
    control->fl_bitmap = 0;
    memset(control->sl_bitmap, 0, FL_INDEX_COUNT * sizeof(control->sl_bitmap[0]));
    for (int i = 0; i < FL_INDEX_COUNT; i++) {
        for (int j = 0; j < SL_INDEX_COUNT; j++) {
            control->blocks[i][j] = &control->block_null;
        }
    }
    return 0;
}