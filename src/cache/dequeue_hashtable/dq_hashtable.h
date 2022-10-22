#pragma once

#ifndef C_HTFH_RT_SEARCH_CACHE_DEQUEUE_HASHTABLE
#define C_HTFH_RT_SEARCH_CACHE_DEQUEUE_HASHTABLE

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "../hashtable/cache_hashtable.h"
#include "dq_ht_entry.h"
#include "../../allocator/alloc_manager.h"

typedef void (*EntryValueDestroyHandler)(AM_ALLOCATOR_PARAM void* entry, void* ctx);

typedef struct DequeueHashTable {
    KeyComparator keyComparator;
    EntryValueDestroyHandler entryDestroyHandler;
    DQHTEntry* head;
    DQHTEntry* tail;
    HashTable* ht;
} DequeueHashTable;

#ifdef DQHT_ENABLE_STRICT
#define DQHT_STRICT_CHECK(dqht) ((dqht)->ht == NULL)
#else
#define DQHT_STRICT_CHECK(dqht) false
#endif

DequeueHashTable* dqht_create(AM_ALLOCATOR_PARAM size_t size, KeyComparator comparator, EntryValueDestroyHandler handler);
void dqht_destroy_handled(AM_ALLOCATOR_PARAM DequeueHashTable* dqht, EntryValueDestroyHandler handler, void* callbackState);
void dqht_destroy(AM_ALLOCATOR_PARAM DequeueHashTable* dqht);

void* dqht_get(DequeueHashTable* dqht, const char* key);
int dqht_insert(AM_ALLOCATOR_PARAM DequeueHashTable* dqht, const char* key, void* value);
void* dqht_remove(AM_ALLOCATOR_PARAM DequeueHashTable* dqht, const char* key);

void* dqht_get_front(DequeueHashTable* dqht);
int dqht_push_front(AM_ALLOCATOR_PARAM DequeueHashTable* dqht, const char* key, void* value);
void* dqht_pop_front(AM_ALLOCATOR_PARAM DequeueHashTable* dqht);

void* dqht_get_last(DequeueHashTable* dqht);
int dqht_push_last(AM_ALLOCATOR_PARAM DequeueHashTable* dqht, const char* key, void* value);
void* dqht_pop_last(AM_ALLOCATOR_PARAM DequeueHashTable* dqht);

void dqht_print_table(char* name, DequeueHashTable* dqht);

#ifdef __cplusplus
};
#endif

#endif // C_HTFH_RT_SEARCH_CACHE_DEQUEUE_HASHTABLE