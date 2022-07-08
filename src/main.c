#include <stdlib.h>
#include "cache/dlirs/dlirs.h"
#include "allocator/error/allocator_errno.h"

#define ENABLE_LOGGING
#include "logging/logging.h"


#define print_error(subs, bytes) \
    char msg[100] \
    sprintf(msg, subs, bytes); \
    alloc_perror(msg); \
    return 1

#define HEAP_SIZE (16 * 10000)

int main(int argc, char* argv[]) {
    DLIRS* dlirs = dlirs_create(10, 10, 5, 0.1f);
    if (dlirs == NULL) {
        printf("Could not create table with size 10\n");
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
    for (int i = 0; i < 10; i++) {
        DLIRSEntry* evicted;
        if (dlirs_request(dlirs, to_store[i], &values[i], evicted) != 0) {
            LOG(ERROR, STDOUT, "Unable to request cache population for [%s: %d]", to_store[i], values[i]);
            return 1;
        }
    }
    dlirs_destroy(dlirs);
    return 0;
}