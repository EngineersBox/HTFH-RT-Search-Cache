#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_CACHE_HASHTABLE_H
#define HTFH_RT_SEARCH_CACHE_CACHE_HASHTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stddef.h>
#include "cache_ht_entry.h"

typedef struct HashTable {
    int size;
    int count;
    HTEntry** items;
} HashTable;

HashTable* ht_create(size_t size);
void ht_destroy(HashTable* ht);
int ht_resize(HashTable* ht);

void* ht_get(HashTable* ht, const char* key);
void* ht_insert(HashTable* ht, const char* key, void* value);
void* ht_delete(HashTable* ht, const char* key);

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_CACHE_HASHTABLE_H
