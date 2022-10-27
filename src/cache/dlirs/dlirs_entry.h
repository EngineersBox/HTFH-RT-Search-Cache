#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_DLIRS_ENTRY_H
#define HTFH_RT_SEARCH_CACHE_DLIRS_ENTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <inttypes.h>
#include <stdbool.h>

#include "../../allocator/alloc_manager.h"
#include "../../allocator/reference_counter.h"

typedef void (*ValueDestroy)(AM_ALLOCATOR_PARAM void* value);

typedef struct DLIRSEntry {
    bool is_LIR;
    bool is_demoted;
    bool in_cache;
    size_t length;
    ReferenceCounter reference_counter;
    char* key;
    void* value;
    ValueDestroy value_destroy_handler;
} DLIRSEntry;

typedef void* (*ValueCopy)(AM_ALLOCATOR_PARAM void* old_value);

DLIRSEntry* dlirs_entry_create_full(AM_ALLOCATOR_PARAM const char* key, void* value, bool is_LIR, bool in_cache, ValueDestroy value_destroy_handler);
DLIRSEntry* dlirs_entry_create(AM_ALLOCATOR_PARAM const char* key, void* value, ValueDestroy value_destroy_handler);
DLIRSEntry* dlirs_entry_reference_copy(AM_ALLOCATOR_PARAM DLIRSEntry* other);
DLIRSEntry* dlirs_entry_copy(AM_ALLOCATOR_PARAM DLIRSEntry* other, ValueCopy value_copy_handler, ValueDestroy value_destroy_handler);
void dlirs_entry_destroy(AM_ALLOCATOR_PARAM DLIRSEntry* entry);

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_DLIRS_ENTRY_H
