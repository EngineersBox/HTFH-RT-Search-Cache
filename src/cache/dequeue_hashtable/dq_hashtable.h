#pragma once

#ifndef _C_HTFH_RT_SEARCH_CACHE_DEQUEUE_HASHTABLE_
#define _C_HTFH_RT_SEARCH_CACHE_DEQUEUE_HASHTABLE_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include <stdbool.h>

#include "../hashtable/cache_hashtable.h"
#include "dq_ht_entry.h"

typedef struct DequeueHashTable {
    DQHTEntry* head;
    DQHTEntry* tail;

    HashTable* ht;
} DequeueHashTable;

typedef void (*EntryValueDestroyHandler)(void*, void*);

#ifdef DQHT_ENABLE_STRICT
#define DQHT_STRICT_CHECK(dqht) (dqht->ht == NULL)
#else
#define DQHT_STRICT_CHECK(dqht) false
#endif

DequeueHashTable* dqht_create(size_t size);
void dqht_destroy_handled(DequeueHashTable* dqht, EntryValueDestroyHandler handler, void* callbackState);
void dqht_destroy(DequeueHashTable* dqht);

void* dqht_get(DequeueHashTable* dqht, const char* key);
int dqht_insert(DequeueHashTable* dqht, const char* key, void* value);
void* dqht_remove(DequeueHashTable* dqht, const char* key);

void* dqht_get_front(DequeueHashTable* dqht);
int dqht_push_front(DequeueHashTable* dqht, const char* key, void* value);
void* dqht_pop_front(DequeueHashTable* dqht);

void* dqht_get_last(DequeueHashTable* dqht);
int dqht_push_last(DequeueHashTable* dqht, const char* key, void* value);
void* dqht_pop_last(DequeueHashTable* dqht);

void dqht_print_table(char* name, DequeueHashTable* dqht);

#ifdef __cplusplus
};
#endif

#endif // _C_HTFH_RT_SEARCH_CACHE_DEQUEUE_HASHTABLE_