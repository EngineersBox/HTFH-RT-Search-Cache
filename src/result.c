#include "result.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

void result_copy_no_results(Result* new_result, Result* old_result) {
    new_result->term1Found = old_result->term1Found;
    new_result->term2Found = old_result->term2Found;
    new_result->offset2 = old_result->offset2;
    new_result->match_count = old_result->match_count;
}

Result* result_copy_sized(AM_ALLOCATOR_PARAM Result* old_result, size_t topK) {
    Result* new_result = (Result*) am_malloc(sizeof(*new_result));
    if (new_result == NULL) {
        return NULL;
    }
    result_copy_no_results(new_result, old_result);
    if (topK > 0 && topK <= new_result->match_count) {
        new_result->match_count = topK;
    }
    // TODO: Separate orderings in query handlers and implement copy here
    new_result->ordering = NULL;
    new_result->results = (PostIt*) am_calloc(new_result->match_count, sizeof(PostIt));
    if (new_result->results == NULL) {
        return NULL;
    }
    memcpy(new_result->results, old_result->results, new_result->match_count * sizeof(PostIt));
    return new_result;
}

Result* result_copy(AM_ALLOCATOR_PARAM Result* old_result) {
    return result_copy_sized(AM_ALLOCATOR_ARG old_result, -1);
}

void result_destroy(AM_ALLOCATOR_PARAM Result* result) {
    if (result == NULL || result->results == NULL) {
        return;
    }
    if (result->ordering != NULL) {
        am_free(result->ordering);
    }
    am_free(result->results);
    am_free(result);
}