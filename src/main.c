#include <stdlib.h>
#include "cache/cache.h"
#include "error/allocator_errno.h"

struct TestStruct {
    int value;
    char str[18];
};

#define print_error(subs, bytes) \
    char *msg = calloc(100, sizeof(*msg)); \
    sprintf(msg, subs, bytes); \
    alloc_perror(msg); \
    free(msg); \
    return 1

#define HEAP_SIZE (16 * 10000)

int main(int argc, char* argv[]) {
    Cache* cache = cache_new(HEAP_SIZE);
    if (cache == NULL) {
        alloc_perror("Initialisation of cache failed for heap size 16*10000 bytes: ");
        return 1;
    }


    if (cache_destroy(cache) != 0) {
        alloc_perror("");
        return 1;
    }

    return 0;
}