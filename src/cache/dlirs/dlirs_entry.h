#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_DLIRS_ENTRY_H
#define HTFH_RT_SEARCH_CACHE_DLIRS_ENTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <inttypes.h>
#include <stdbool.h>

typedef struct DLIRSEntry {
    bool is_LIR;
    bool is_demoted;
    bool in_cache;
    size_t length;
    char* key;
    void* value;
} DLIRSEntry;

DLIRSEntry* dlirs_entry_create_full(const char* key, void* value, bool is_LIR, bool in_cache);
DLIRSEntry* dlirs_entry_create(const char* key, void* value);
DLIRSEntry* dlirs_entry_copy(DLIRSEntry* other);
void dlirs_entry_destroy(DLIRSEntry* entry);

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_DLIRS_ENTRY_H
