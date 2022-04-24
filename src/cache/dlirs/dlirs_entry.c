#include "dlirs_entry.h"

#include <stdlib.h>

DLIRSEntry* dlirs_entry_create_full(void* value, bool is_LIR, bool in_cache) {
    DLIRSEntry* entry = malloc(sizeof(*entry));
    if (entry == NULL) {
        return NULL;
    }
    entry->value = value;
    entry->is_LIR = is_LIR;
    entry->in_cache = in_cache;
    entry->is_demoted = false;
    return entry;
}

DLIRSEntry* dlirs_entry_create(void* value) {
    return dlirs_entry_create_full(value, false, true);
}

void dlirs_entry_destroy(DLIRSEntry* entry) {
    if (entry == NULL) {
        return NULL;
    }
    free(entry);
}