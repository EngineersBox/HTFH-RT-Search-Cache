#include "dq_ht_entry.h"

#include <string.h>
//#include <stdio.h>
#include <stdlib.h>
#include "../cache_key.h"
#include "logging/logging.h"

DQHTEntry* dqhtentry_create(AM_ALLOCATOR_PARAM const char* key, void* ptr) {
    return dqhtentry_create_full(AM_ALLOCATOR_ARG key, ptr, NULL, NULL);
}

DQHTEntry* dqhtentry_create_full(AM_ALLOCATOR_PARAM const char* key, void* ptr, DQHTEntry* prev, DQHTEntry* next) {
    DQHTEntry* entry = (DQHTEntry*) am_malloc(sizeof(*entry));
    if (entry == NULL) {
        return NULL;
    }
    entry->length = key_size(key);
    entry->key = key_clone(AM_ALLOCATOR_ARG key);
    if (entry->key == NULL) {
        return NULL;
    }
    char* keyValue = key_sprint(entry->key);
    DEBUG("NEW ENTRY [Key: %s]\n", key_sprint(entry->key));
    free(keyValue);
    entry->ptr = ptr;
    DEBUG("NEW ENTRY [Ptr: %p]\n", entry->ptr);
    entry->prev = prev;
    DEBUG("NEW ENTRY [Prev: %p]\n", entry->prev);
    entry->next = next;
    DEBUG("NEW ENTRY [Next: %p]\n", entry->next);
    entry->index = 0;
    return entry;
}

void dqhtentry_destroy(AM_ALLOCATOR_PARAM DQHTEntry* entry) {
    if (entry == NULL || entry->key == NULL) {
        return;
    }
    char* keyValue = key_sprint(entry->key);
    DEBUG("FREEING ENTRY [Key %s] [Ptr %p]", keyValue, entry->ptr);
    free(keyValue);
    am_free(entry->key);
    entry->key = NULL;
    entry->next = NULL;
    entry->prev = NULL;
    am_free(entry);
}