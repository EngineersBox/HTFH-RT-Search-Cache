#include <stdlib.h>
#include "cache/dlirs/dlirs.h"
#include "allocator/error/allocator_errno.h"
#include "cache/dequeue_hashtable/dq_hashtable.h"

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
    DequeueHashTable * table = dqht_create(10);
    if (table == NULL) {
        printf("Could not create table with size 10 and max_jumps 5\n");
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
        if (dqht_set(table, to_store[i], &values[i]) != 0) {
            printf("Could not insert entry: [%s: %d]\n", to_store[i], values[i]);
            return 1;
        }
        printf("=> Inserted entry %d\n", i);
    }
    printf("== Inserted entries ==\n");
    for (int i = 0; i < 10; i++) {
        int* value;
        if ((value = dqht_get(table, to_store[i])) == NULL) {
            printf("Could not get entry: [%s]\n", to_store[i]);
            return 1;
        } else if (*value != values[i]) {
            printf("Value %d did not match expected: %d", *value, values[i]);
            return 1;
        }
        printf("=> Retrieved entry %d: %d\n", i, *value);
    }
    printf("== Retrieved entries ==\n");
    for (int i = 0; i < 10; i++) {
        if (dqht_remove(table, to_store[i])  != 0) {
            printf("Could not delete entry: [%s]\n", to_store[i]);
            return 1;
        }
        printf("=> Deleted entry %d\n", i);
    }
    printf("== Deleted entries ==\n");
    dqht_destroy(table);

    return 0;
}