#include "dq_ht_entry.h"

#include <string.h>
#include <stdlib.h>

DQHTEntry* dqhtentry_create(const char* key, void* ptr) {
    return dqhtentry_create_full(key, ptr, NULL, NULL);
}

DQHTEntry* dqhtentry_create_full(const char* key, void* ptr, DQHTEntry* prev, DQHTEntry* next) {
    DQHTEntry* entry = malloc(sizeof(*entry));
    entry->length = strlen(key);
    entry->key = malloc(entry->length + 1);
    strncpy(entry->key, key, entry->length);
    entry->ptr = ptr;
    entry->prev = prev;
    entry->next = next;
    return entry;
}

void dqhtentry_destroy(DQHTEntry* entry) {
    if (entry == NULL) {
        return;
    }
    free(entry->key);
    free(entry);
}