#include "dq_ht_entry.h"

#include <string.h>
#include <stdlib.h>

DQHTEntry* dqhtentry_create(const char* key, void* ptr) {
    return dqhtentry_create_full(key, ptr, NULL, NULL);
}

DQHTEntry* dqhtentry_create_full(const char* key, void* ptr, DQHTEntry* prev, DQHTEntry* next) {
    DQHTEntry* entry = malloc(sizeof(*entry));
    if (entry == NULL) {
        return NULL;
    }
    entry->length = strlen(key);
    entry->key = calloc(entry->length + 1, sizeof(char));
    if (entry->key == NULL) {
        return NULL;
    }
    strncpy(entry->key, key, entry->length);
    entry->ptr = ptr;
    entry->prev = prev;
    entry->next = next;
    entry->index = 0;
    return entry;
}

void dqhtentry_destroy(DQHTEntry* entry) {
    if (entry == NULL || entry->key == NULL) {
        return;
    }
    free(entry->key);
    entry->key = NULL;
    entry->next = entry->prev = NULL;
    entry->index = 0;
    free(entry);
    entry = NULL;
}