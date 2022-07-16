#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_DLIRS_ENTRY_H
#define HTFH_RT_SEARCH_CACHE_DLIRS_ENTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <inttypes.h>
#include <stdbool.h>

#include "../allocator/alloc_manager.h"

typedef struct DLIRSEntry {
    bool is_LIR;
    bool is_demoted;
    bool in_cache;
    size_t length;
    char* key;
    void* value;
} DLIRSEntry;

DLIRSEntry* dlirs_entry_create_full(AM_ALLOCATOR_PARAM const char* key, void* value, bool is_LIR, bool in_cache);
DLIRSEntry* dlirs_entry_create(AM_ALLOCATOR_PARAM const char* key, void* value);
DLIRSEntry* dlirs_entry_copy(AM_ALLOCATOR_PARAM DLIRSEntry* other);
void dlirs_entry_destroy(AM_ALLOCATOR_PARAM DLIRSEntry* entry);

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_DLIRS_ENTRY_H
