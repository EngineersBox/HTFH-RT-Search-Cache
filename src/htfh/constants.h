#pragma once

#ifndef _C_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_CONSTANTS_
#define _C_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_CONSTANTS_

#ifdef __cplusplus
extern "C" {
#endif

enum htfh_public {
    /* log2 of number of linear subdivisions of block sizes. Larger
    ** values require more memory in the control structure. Values of
    ** 4 or 5 are typical.
    */
    SL_INDEX_COUNT_LOG2 = 5,
};

enum htfh_private {
#if defined (ARCH_64_BIT)
    /* All allocation sizes and addresses are aligned to 8 bytes. */
    ALIGN_SIZE_LOG2 = 3,
#else
    /* All allocation sizes and addresses are aligned to 4 bytes. */
    ALIGN_SIZE_LOG2 = 2,
#endif
    ALIGN_SIZE = (1 << ALIGN_SIZE_LOG2),
#if defined (ARCH_64_BIT)
    FL_INDEX_MAX = 32,
#else
    FL_INDEX_MAX = 30,
#endif
    SL_INDEX_COUNT = (1 << SL_INDEX_COUNT_LOG2),
    FL_INDEX_SHIFT = (SL_INDEX_COUNT_LOG2 + ALIGN_SIZE_LOG2),
    FL_INDEX_COUNT = (FL_INDEX_MAX - FL_INDEX_SHIFT + 1),
    SMALL_BLOCK_SIZE = (1 << FL_INDEX_SHIFT),
};

#ifdef __cplusplus
};
#endif

#endif // _C_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_CONSTANTS_