#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "cache/simple/random.h"

#define HEAP_SIZE (16 * 10000)

int random_test_main(int argc, char* argv[]) {
#ifdef HTFH_ALLOCATOR
    Allocator* allocator = htfh_create(HEAP_SIZE);
    if (allocator == NULL) {
        printf("Unable to create HTFH with size of %d", HEAP_SIZE);
        return 1;
    }
#endif
    srand((unsigned int)time(NULL));
    RandomCache* rc = rc_create(AM_ALLOCATOR_ARG 10, 10, NULL);
    if (rc == NULL) {
        printf("Unable to create RandomCache with size of 10");
        return 1;
    }
    char* to_store[10] = {
        "test1",
        "other",
        "more",
        "something",
        "yeahd",
        "dgsdfs",
        "ehjigdss",
        "oijfguod",
        "ngujhdw",
        "doij42od"
    };
    int values[10] = {
        56135,
        7345,
        5246,
        16473,
        9537,
        24675,
        55363,
        426723,
        957357,
        64526
    };
    rc_destroy(AM_ALLOCATOR_ARG rc);
    return 0;
}