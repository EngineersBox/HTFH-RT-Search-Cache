#include "heap_entry.h"

#include <string.h>

HeapEntry* he_create(const char* key, void* value) {
    HeapEntry* entry = malloc(sizeof(*entry));
    if (entry == NULL) {
        return NULL;
    }
    entry->length = strlen(key);
    entry->key = calloc(entry->length + 1, sizeof(char));
    if (entry->key == NULL) {
        return NULL;
    }
    strncpy(entry->key, key, entry->length);
    entry->value = value;
    entry->index = -1;
    return entry;
}

void he_destroy(HeapEntry* entry) {
    if (entry == NULL) {
        return;
    }
    free(entry);
}

// -1 = failure, 0 = less than, 1 = not less than
int he_less_than(HeapEntry* entry1, HeapEntry* entry2) {
    if (entry1 == NULL || entry2 == NULL) {
        return -1; // Failure
    } else if (entry1->value != NULL && entry2->value == NULL) {
        return 1; // Only entry1 has a value so this is not less than
    } else if (entry1->value == NULL && entry2->value != NULL) {
        return 0; // Only entry2 has a value so this is less than
    }
    // Compare with however comparison should operate on the value pointed to.
    return *((int*)entry1->value) < *((int*)entry2->value);
}