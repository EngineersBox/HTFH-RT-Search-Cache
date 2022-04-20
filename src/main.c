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
    Allocator* alloc = htfh_create(HEAP_SIZE);
    if (alloc == NULL) {
        alloc_perror("Initialisation failed for heap size 16*10000 bytes: ");
        return 1;
    }
    struct TestStruct* test_struct = htfh_calloc(alloc, 2, sizeof(*test_struct));
    if (test_struct == NULL) {
        print_error("Failed to allocate %zu bytes for TestStruct: ", sizeof(*test_struct));
    }
    test_struct[0].value = 42;
    strncpy(test_struct[0].str, "abcdefghijklmnopqr", 18);

    test_struct[1].value = 37;
    strncpy(test_struct[1].str, "kejcufnetisprmguch", 18);

    printf("Test struct[0]: [Value: %d] [Str: %s]\n", test_struct[0].value, test_struct[0].str);
    printf("Test struct[1]: [Value: %d] [Str: %s]\n", test_struct[0].value, test_struct[0].str);

    struct TestStruct* test_struct2 = htfh_malloc(alloc, sizeof(*test_struct2));
    if (test_struct2 == NULL) {
        print_error("Failed to allocate %zu bytes for TestStruct2: ", sizeof(*test_struct2));
    }

    if (htfh_free(alloc, test_struct) != 0) {
        alloc_perror("");
        return 1;
    }

    test_struct2->value = 84;
    strncpy(test_struct2->str, "012345678901234567", 18);

    printf("Test struct 2:  [Value: %d] [Str: %s]\n", test_struct2->value, test_struct2->str);
    printf("Test struct[0]: [Value: %d] [Str: %s]\n", test_struct[0].value, test_struct[0].str);

    if (htfh_free(alloc, test_struct2) != 0) {
        alloc_perror("");
        return 1;
    }

    if (htfh_destroy(alloc) != 0) {
        alloc_perror("");
        return 1;
    }


    return 0;
}