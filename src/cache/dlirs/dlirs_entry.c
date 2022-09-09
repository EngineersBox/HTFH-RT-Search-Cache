#include "dlirs_entry.h"

#include <stdlib.h>
#include <string.h>

#include "../../logging/logging.h"

DLIRSEntry* dlirs_entry_create_full(AM_ALLOCATOR_PARAM const char* key, void* value, bool is_LIR, bool in_cache) {
    DLIRSEntry* entry = am_malloc(sizeof(*entry));
    if (entry == NULL) {
        return NULL;
    }
    entry->length = strlen(key);
    entry->key = am_calloc(entry->length + 1, sizeof(char));
    if (entry->key == NULL) {
        return NULL;
    }
    strncpy(entry->key, key, entry->length);
    entry->value = value;
    entry->is_LIR = is_LIR;
    entry->in_cache = in_cache;
    entry->is_demoted = false;
    return entry;
}

DLIRSEntry* dlirs_entry_create(AM_ALLOCATOR_PARAM const char* key, void* value) {
    DLIRSEntry* entry = dlirs_entry_create_full(AM_ALLOCATOR_ARG key, value, false, true);
    TRACE("CREATED NEW DLIRSEntry: %p [%s: %p]", entry, key, value);
    return entry;
}

DLIRSEntry* dlirs_entry_copy(AM_ALLOCATOR_PARAM DLIRSEntry* other) {
    if (other == NULL || other->key == NULL) {
        return NULL;
    }
    DLIRSEntry* entry = dlirs_entry_create_full(
        AM_ALLOCATOR_ARG
        other->key,
        other->value,
        other->is_LIR,
        other->in_cache
    );
    entry->is_demoted = other->is_demoted;
    entry->length = other->length;
    return entry;
}

void dlirs_entry_destroy(AM_ALLOCATOR_PARAM DLIRSEntry* entry) {
    if (entry == NULL || entry->key == NULL) {
        return;
    }
//    TRACE("DESTROYING DLIRSEntry: %p [%s: %p]", entry, entry->key, entry->value);
    am_free(entry->key);
    entry->key = NULL;
    am_free(entry);
    entry = NULL;
}