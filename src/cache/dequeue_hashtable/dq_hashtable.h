#pragma once

#ifndef _C_HTFH_RT_SEARCH_CACHE_DEQUEUE_HASHTABLE_
#define _C_HTFH_RT_SEARCH_CACHE_DEQUEUE_HASHTABLE_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "dq_ht_entry.h"

typedef struct DequeueHashTable {
    int size;
    int count;

    DQHTEntry* head;
    DQHTEntry* tail;

    DQHTEntry** items;
} DequeueHashTable;

DequeueHashTable* dqht_create(size_t size);
void dqht_destroy(DequeueHashTable* dqht);
int dqht_resize(DequeueHashTable* dqht);

void* dqht_get(DequeueHashTable* dqht, const char* key);
int dqht_set(DequeueHashTable* dqht, const char* key, void* value);
int dqht_remove(DequeueHashTable* dqht, const char* key);

void* dqht_first(DequeueHashTable* dqht);
int dqht_push_first(DequeueHashTable* dqht, const char* key, void* value);
void* dqht_pop_first(DequeueHashTable* dqht);

void* dqht_last(DequeueHashTable* dqht);
void* dqht_pop_last(DequeueHashTable* dqht);

#ifdef __cplusplus
};
#endif

#endif // _C_HTFH_RT_SEARCH_CACHE_DEQUEUE_HASHTABLE_