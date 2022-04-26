#include "dlirs_entry.h"

#include <stdlib.h>
#include <string.h>

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
    return dlirs_entry_create_full(key, value, false, true);
}

void dlirs_entry_destroy(DLIRSEntry* entry) {
    if (entry != NULL) {
        free(entry);
    }
}