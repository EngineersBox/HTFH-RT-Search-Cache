#include "dlirs_entry.h"

#include <stdlib.h>
#include <string.h>

#include "../../logging/logging.h"
#include "../cache_key.h"
#include "../result.h"
#include "../../allocator/error/allocator_errno.h"

DLIRSEntry* dlirs_entry_create_full(AM_ALLOCATOR_PARAM const char* key, void* value, bool is_LIR, bool in_cache) {
    DLIRSEntry* entry = (DLIRSEntry*) am_malloc(sizeof(*entry));
    if (entry == NULL) {
        return NULL;
    }
    entry->length = key_size(key);
    entry->key = key_clone(AM_ALLOCATOR_ARG key);
    if (entry->key == NULL) {
        return NULL;
    }
    entry->value = value;
    entry->is_LIR = is_LIR;
    entry->in_cache = in_cache;
    entry->is_demoted = false;
    return entry;
}

DLIRSEntry* dlirs_entry_create(AM_ALLOCATOR_PARAM const char* key, void* value) {
    return dlirs_entry_create_full(AM_ALLOCATOR_ARG key, value, false, true);
}

void* default_copy_handler(AM_ALLOCATOR_PARAM void* old_value) {
    return old_value;
}

DLIRSEntry* dlirs_entry_copy(AM_ALLOCATOR_PARAM DLIRSEntry* other, ValueCopy copy_handler) {
    if (other == NULL || other->key == NULL) {
        return NULL;
    }
    DLIRSEntry* entry = dlirs_entry_create_full(
        AM_ALLOCATOR_ARG
        other->key,
        copy_handler == NULL ? default_copy_handler(AM_ALLOCATOR_ARG other->value) : copy_handler(AM_ALLOCATOR_ARG other->value),
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
    char* keyValue = key_sprint(entry->key);
    TRACE("DESTROYING DLIRSEntry: %p [%s: %p]", entry, keyValue, entry->value);
    free(keyValue);
    am_free(entry->key);
    entry->key = NULL;
    am_free(entry);
}