#pragma once

#ifndef _C_HTFH_RT_SEARCH_CACHE_DEQUEUE_HASHTABLE_
#define _C_HTFH_RT_SEARCH_CACHE_DEQUEUE_HASHTABLE_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "../hashtable/cache_hashtable.h"
#include "../hashtable/dq_ht_entry.h"

typedef struct DequeueHashTable {
    DQHTEntry* head;
    DQHTEntry* tail;

    HashTable* ht;
} DequeueHashTable;

DequeueHashTable* dqht_create(size_t size);
void dqht_destroy(DequeueHashTable* dqht);

void* dqht_get(DequeueHashTable* dqht, const char* key);
int dqht_insert(DequeueHashTable* dqht, const char* key, void* value);
int dqht_remove(DequeueHashTable* dqht, const char* key);

void* dqht_get_front(DequeueHashTable* dqht);
int dqht_push_front(DequeueHashTable* dqht, const char* key, void* value);
void* dqht_pop_front(DequeueHashTable* dqht);

void* dqht_get_last(DequeueHashTable* dqht);
int dqht_push_last(DequeueHashTable* dqht, const char* key, void* value);
void* dqht_pop_last(DequeueHashTable* dqht);

void dqht_print_table(DequeueHashTable* dqht);

#ifdef __cplusplus
};
#endif

#endif // _C_HTFH_RT_SEARCH_CACHE_DEQUEUE_HASHTABLE_