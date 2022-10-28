#include "dlirs_entry.h"

#include <stdlib.h>
#include <string.h>

#include "../../logging/logging.h"
#include "../cache_key.h"
#include "../result.h"
#include "../../allocator/error/allocator_errno.h"

DLIRSEntry* dlirs_entry_create_full(AM_ALLOCATOR_PARAM const char* key, void* value, bool is_LIR, bool in_cache, ValueDestroy value_destroy_handler) {
    DLIRSEntry* entry = (DLIRSEntry*) am_malloc(sizeof(*entry));
    if (entry == NULL) {
        return NULL;
    }
    entry->value_destroy_handler = value_destroy_handler;
    rc_init(entry->reference_counter);
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

DLIRSEntry* dlirs_entry_create(AM_ALLOCATOR_PARAM const char* key, void* value, ValueDestroy value_destroy_handler) {
    return dlirs_entry_create_full(AM_ALLOCATOR_ARG key, value, false, true, value_destroy_handler);
}

void* default_copy_handler(AM_ALLOCATOR_PARAM void* old_value) {
    return old_value;
}

DLIRSEntry* dlirs_entry_reference_copy(DLIRSEntry* other) {
    if (other == NULL || other->key == NULL) {
        return NULL;
    }
    rc_increment(other->reference_counter);
    return other;
}

DLIRSEntry* dlirs_entry_copy(AM_ALLOCATOR_PARAM DLIRSEntry* other, ValueCopy value_copy_handler, ValueDestroy value_destroy_handler) {
    if (other == NULL || other->key == NULL) {
        return NULL;
    }
    DLIRSEntry* entry = dlirs_entry_create_full(
        AM_ALLOCATOR_ARG
        other->key,
        value_copy_handler == NULL ? default_copy_handler(AM_ALLOCATOR_ARG other->value) : value_copy_handler(AM_ALLOCATOR_ARG other->value),
        other->is_LIR,
        other->in_cache,
        value_destroy_handler
    );
    entry->is_demoted = other->is_demoted;
    entry->length = other->length;
    return entry;
}

void dlirs_entry_destroy(AM_ALLOCATOR_PARAM DLIRSEntry* entry) {
    if (entry == NULL || entry->key == NULL) {
        return;
    }
    rc_decrement(entry->reference_counter);
    char* keyValue = key_sprint(entry->key);
    TRACE("DLIRSEntry %s reference counter decremented %d -> %d", keyValue, entry->reference_counter + 1, entry->reference_counter);
    free(keyValue);
    if (!rc_last(entry->reference_counter)) {
        return;
    }
    keyValue = key_sprint(entry->key);
    TRACE("DESTROYING DLIRSEntry: %p [%s: %p]", entry, keyValue, entry->value);
    free(keyValue);
    am_free(entry->key);
    entry->key = NULL;
    entry->value_destroy_handler(AM_ALLOCATOR_ARG entry->value);
    am_free(entry);
}