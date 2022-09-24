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
    unsigned long long int offset1;
    unsigned long long int offset2;
    size_t match_count;
    PostIt* results;
} Result;

static void result_copy_no_results(Result* new_result, Result* old_result) {
    new_result->term1Found = old_result->term1Found;
    new_result->term2Found = old_result->term2Found;
    new_result->offset1 = old_result->offset1;
    new_result->offset2 = old_result->offset2;
    new_result->match_count = old_result->match_count;
}

static Result* result_copy(AM_ALLOCATOR_PARAM Result* old_result) {
    Result* new_result = (Result*) am_malloc(sizeof(*new_result));
    if (new_result == NULL) {
        return NULL;
    }
    result_copy_no_results(new_result, old_result);
    new_result->results = (PostIt*) am_calloc(new_result->match_count, sizeof(PostIt));
    if (new_result->results == NULL) {
        return NULL;
    }
    memcpy(new_result->results, old_result->results, new_result->match_count * sizeof(PostIt));
    return new_result;
}

#ifdef __cplusplus
};
#endif

#endif //LSIP_RESULT_H
