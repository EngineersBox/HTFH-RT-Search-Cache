#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_HEAPTABLE_H
#define HTFH_RT_SEARCH_CACHE_HEAPTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>

#include "../hashtable/cache_hashtable.h"
#include "heap_entry.h"

typedef struct HeapTable {
    HashTable* ht;
    HeapEntry** heap;
} HeapTable;

HeapTable* hpt_create(size_t ht_size);
void hpt_destroy(HeapTable* hpt);

int hpt_contains(HeapTable* hpt, const char* key);
void* hpt_get(HeapTable* hpt, const char* key);
int hpt_insert(HeapTable* hpt, const char* key, void* value);
int hpt_delete(HeapTable* hpt, const char* key);

void* hpt_min(HeapTable* hpt);
void* hpt_popMin(HeapTable* hpt);

HeapEntry* hpt_parent(HeapTable* hpt, size_t index);
HeapEntry* hpt_child_left(HeapTable* hpt, size_t index);
HeapEntry* hpt_child_right(HeapTable* hpt, size_t index);

void hpt_heapify_up(HeapTable* hpt, size_t index);
void hpt_heapify(HeapTable* hpt, size_t index);

void hpt_remove(HeapTable* hpt, const char* key);
void hpt_push(HeapTable* hpt, const char* key, void* value);
void hpt_update(HeapTable* hpt const char* key, void* value);

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_HEAPTABLE_H
