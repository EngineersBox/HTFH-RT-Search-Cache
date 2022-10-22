#include <stdlib.h>
#include "allocator/error/allocator_errno.h"
#include "cache/dequeue_hashtable/dq_hashtable.h"
#include "cache/cache_key.h"

struct TestStruct {
    int value;
    char str[18];
};

#define print_error(subs, bytes) \
    char msg[100]; \
    sprintf(msg, subs, bytes); \
    alloc_perror(msg); \
    return 1

#define HEAP_SIZE (16 * 10000)

void destroy_entry_handler(AM_ALLOCATOR_PARAM void* entry, void* _ignored) {
    if (entry == NULL) {
        return;
    }
    am_free(entry);
}

int dqht_test_main(int argc, char* argv[]) {
#if ALLOCATOR_TYPE > 0
#if ALLOCATOR_TYPE == 1
    Allocator* allocator = htfh_create(HEAP_SIZE);
#elif ALLOCATOR_TYPE == 2
    GlibcAllocator* allocator = gca_create(HEAP_SIZE);
#endif
    if (allocator == NULL) {
        printf("Unable to create allocator with heap size of %d", HEAP_SIZE);
        return 1;
    }
#endif
    DequeueHashTable* table = dqht_create(AM_ALLOCATOR_ARG 10, NULL, (EntryValueDestroyHandler) destroy_entry_handler);
    if (table == NULL) {
        printf("Could not create table with size 10\n");
        return 1;
    }

    char* to_store[10] = {
        key_create('0', "test1", NULL),
        key_create('0', "other", NULL),
        key_create('0', "more", NULL),
        key_create('0', "something", NULL),
        key_create('0', "yeahd", NULL),
        key_create('0', "dgsdfs", NULL),
        key_create('0', "ehjigdss", NULL),
        key_create('0', "oijfguod", NULL),
        key_create('0', "ngujhdw", NULL),
        key_create('0', "doij42od", NULL)
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
        if (dqht_push_front(AM_ALLOCATOR_ARG table, to_store[i], &values[i]) != 0) {
            printf("Could not insert entry: [%s: %d]\n", key_sprint(to_store[i]), values[i]);
            return 1;
        }
        if (table->head == NULL && table->tail == NULL) {
            printf("Head: (nil), Tail: (nil)\n");
        } else {
            printf("Head: %d, Tail: %d\n", *((int*)table->head->ptr), *((int*)table->tail->ptr));
        }
        if (i == 0) {
            if (table->tail->prev != NULL) {
                printf("Initial previous at tail should be null\n");
                return 1;
            } else if (table->head->next != NULL) {
                printf("Initial next at head should be null\n");
                return 1;
            }
        } else {
            if (*((int*)table->tail->ptr) != values[0]) {
                printf("Tail was not equal to first element: %d != %d\n", *((int*)table->tail->ptr), values[0]);
                return 1;
            } else if (*((int*)table->head->ptr) != values[i]) {
                printf("Head was not equal to current element: %d != %d\n", *((int*)table->head->ptr), values[i]);
                return 1;
            }
        }
        printf("=> Inserted entry %d\n", i);
    }
    if (table->ht->count != 10) {
        printf("Count was not 10: %zu\n", table->ht->count);
        return 1;
    }
    dqht_print_table("BEFORE GET:", table);
    printf("== Inserted entries ==\n");
    for (int i = 0; i < 10; i++) {
        int* value;
        if ((value = dqht_get(table, to_store[i])) == NULL) {
            printf("Could not get entry: [%s]\n", key_sprint(to_store[i]));
            return 1;
        } else if (*value != values[i]) {
            printf("Value %d did not match expected: %d\n", *value, values[i]);
            return 1;
        }
        printf("=> Retrieved entry %d: %d\n", i, *value);
    }
    dqht_print_table("BEFORE DELETE", table);
    printf("== Retrieved entries ==\n");
    for (int i = 0; i < 10; i++) {
        if (dqht_pop_last(AM_ALLOCATOR_ARG table)  == NULL) {
            printf("Could not delete entry: [%s]\n", key_sprint(to_store[i]));
            return 1;
        }
        printf("=> Deleted entry %d\n", i);
    }
    if (table->ht->count != 0) {
        printf("Count was not 0: %zu\n", table->ht->count);
        return 1;
    }
    printf("== Deleted entries ==\n");
     dqht_print_table("FINAL", table);
    dqht_destroy(AM_ALLOCATOR_ARG table);

#if ALLOCATOR_TYPE == 1
    if (htfh_destroy(allocator) != 0) {
        printf("Unable to destroy allocator");
        return 1;
    }
#elif AALLOCATOR_TYPE == 2
    if (gca_destroy(allocator) != 0) {
        printf("Unable to destroy allocator");
        return 1;
    }
#endif

    return 0;
}