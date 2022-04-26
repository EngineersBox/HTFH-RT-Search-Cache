#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "cache/simple/random.h"

int random_test_main(int argc, char* argv[]) {
    srand((unsigned int)time(NULL));
    RandomCache* rc = rc_create(10, 10);
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
    rc_destroy(rc);
    return 0;
}