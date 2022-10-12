#pragma once

#ifndef LSIP_RESULT_H
#define LSIP_RESULT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdbool.h>

#include "allocator/alloc_manager.h"

typedef struct PostIt {
    long long int dn;
    unsigned int wc;
} PostIt;

typedef struct __attribute__((packed, aligned(1))) Result {
    bool term1Found;
    bool term2Found;
    unsigned long long int offset2;
    size_t match_count;
    size_t* ordering;
    PostIt* results;
} Result;

void result_copy_no_results(Result* new_result, Result* old_result);
Result* result_copy_sized(AM_ALLOCATOR_PARAM Result* old_result, size_t topK);
Result* result_copy(AM_ALLOCATOR_PARAM Result* old_result);
void result_destroy(AM_ALLOCATOR_PARAM Result* result);

#ifdef __cplusplus
};
#endif

#endif //LSIP_RESULT_H
