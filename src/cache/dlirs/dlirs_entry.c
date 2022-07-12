#include "dlirs_entry.h"

#include <stdlib.h>
#include <string.h>

#define ENABLE_LOGGING
#define LOG_DATETIME_PREFIX
#include "../../logging/logging.h"

DLIRSEntry* dlirs_entry_create_full(const char* key, void* value, bool is_LIR, bool in_cache) {
    DLIRSEntry* entry = malloc(sizeof(*entry));
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
    entry->is_LIR = is_LIR;
    entry->in_cache = in_cache;
    entry->is_demoted = false;
    return entry;
}

DLIRSEntry* dlirs_entry_create(const char* key, void* value) {
    DLIRSEntry* entry = dlirs_entry_create_full(key, value, false, true);
    TRACE("CREATED NEW DLIRSEntry: %p [%s: %p]", entry, key, value);
    return entry;
}

DLIRSEntry* dlirs_entry_copy(DLIRSEntry* other) {
    if (other == NULL || other->key == NULL) {
        return NULL;
    }
    DLIRSEntry* entry = dlirs_entry_create_full(
        other->key,
        other->value,
        other->is_LIR,
        other->in_cache
    );
    entry->is_demoted = other->is_demoted;
    entry->length = other->length;
    return entry;
}

void dlirs_entry_destroy(DLIRSEntry* entry) {
    if (entry == NULL || entry->key == NULL) {
        return;
    }
    TRACE("DESTROYING DLIRSEntry: %p [%s: %p]", entry, entry->key, entry->value);
    free(entry->key);
    entry->key = NULL;
    free(entry);
    entry = NULL;
}