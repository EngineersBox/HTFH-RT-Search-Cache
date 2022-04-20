#include "cache_ht_entry.h"

#include <stdlib.h>

HTEntry* htentry_create(const char* key, void* ptr) {
    HTEntry* entry = malloc(sizeof(*entry));
    entry->length = strlen(key);
    entry->key = malloc(entry->length + 1);
    strncpy(entry->key, key, entry->length);
    entry->ptr = ptr;
    return entry;
}

void htentry_destroy(HTEntry* entry) {
    if (entry == NULL) {
        return;
    }
    free(entry->key);
    free(entry);
}