#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_CACHE_HASHTABLE_H
#define HTFH_RT_SEARCH_CACHE_CACHE_HASHTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>

#include "dq_ht_entry.h"
#include "../allocator/alloc_manager.h"

typedef int (*KeyComparator)(const char* key1, const char* key2);

typedef struct HashTable {
    int size;
    int count;
    KeyComparator comparator;
    DQHTEntry** items;
} HashTable;

HashTable* ht_create(AM_ALLOCATOR_PARAM size_t size, KeyComparator comparator);
void ht_destroy(AM_ALLOCATOR_PARAM HashTable* ht);
int ht_resize(AM_ALLOCATOR_PARAM HashTable* ht);

DQHTEntry* ht_get(HashTable* ht, const char* key);
DQHTEntry* ht_insert(AM_ALLOCATOR_PARAM HashTable* ht, const char* key, void* value);
void* ht_delete(AM_ALLOCATOR_PARAM HashTable* ht, const char* key);

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_CACHE_HASHTABLE_H
