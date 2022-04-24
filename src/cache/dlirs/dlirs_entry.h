#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_DLIRS_ENTRY_H
#define HTFH_RT_SEARCH_CACHE_DLIRS_ENTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct DLIRSEntry {
    bool is_LIR;
    bool is_demoted;
    bool in_cache;
    void* value;
} DLIRSEntry;

DLIRSEntry* dlirs_entry_create_full(void* value, bool is_LIR, bool in_cache);
DLIRSEntry* dlirs_entry_create(void* value);
void dlirs_entry_destroy(DLIRSEntry* entry);

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_DLIRS_ENTRY_H
