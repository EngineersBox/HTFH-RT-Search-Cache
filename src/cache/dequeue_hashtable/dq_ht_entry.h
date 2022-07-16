#pragma once

#ifndef _C_HTFH_RT_SEARCH_CACHE_DEQUEUE_HASHTABLE_ENTRY_
#define _C_HTFH_RT_SEARCH_CACHE_DEQUEUE_HASHTABLE_ENTRY_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "../allocator/alloc_manager.h"

typedef struct DQHTEntry {
    char* key;
    size_t length;
    void* ptr;
    size_t index;
    struct DQHTEntry* prev;
    struct DQHTEntry* next;
} DQHTEntry;

DQHTEntry* dqhtentry_create(AM_ALLOCATOR_PARAM const char* key, void* ptr);
DQHTEntry* dqhtentry_create_full(AM_ALLOCATOR_PARAM const char* key, void* ptr, DQHTEntry* prev, DQHTEntry* next);
void dqhtentry_destroy(AM_ALLOCATOR_PARAM DQHTEntry* entry);

#ifdef __cplusplus
};
#endif

#endif // _C_HTFH_RT_SEARCH_CACHE_DEQUEUE_HASHTABLE_ENTRY_