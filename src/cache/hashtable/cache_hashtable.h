#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_CACHE_HASHTABLE_H
#define HTFH_RT_SEARCH_CACHE_CACHE_HASHTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>

#include "../dequeue_hashtable/dq_ht_entry.h"
#include "../../allocator/alloc_manager.h"

typedef int (*KeyComparator)(const char* key1, const char* key2, void* key2Value);

typedef struct HashTable {
    size_t size;
    size_t count;
    KeyComparator comparator;
    DQHTEntry** items;
} HashTable;

HashTable* ht_create(AM_ALLOCATOR_PARAM size_t size, KeyComparator comparator);
void ht_destroy(AM_ALLOCATOR_PARAM HashTable* ht);
int ht_resize(AM_ALLOCATOR_PARAM HashTable* ht);

DQHTEntry* ht_get(HashTable* ht, const char* key);
// -1: fail, 0: new entry, 1: existing entry
int ht_insert(AM_ALLOCATOR_PARAM HashTable* ht, const char* key, void* value, DQHTEntry** entryAtIndex);

void* ht_delete_entry(AM_ALLOCATOR_PARAM HashTable* ht, size_t index);
void* ht_delete(AM_ALLOCATOR_PARAM HashTable* ht, const char* key);

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_CACHE_HASHTABLE_H
