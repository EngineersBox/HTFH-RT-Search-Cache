#include "dq_hashtable.h"

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "../hashtable/hashing.h"

DequeueHashTable* dqht_create(size_t size) {
    HashTable* ht = ht_create(size);
    if (ht == NULL) {
        return NULL;
    }
    DequeueHashTable* dqht = malloc(sizeof(*dqht));
    if (dqht == NULL) {
        return NULL;
    }
    dqht->head = NULL;
    dqht->tail = NULL;
    dqht->ht = ht;
    return dqht;
}

void dqht_destroy(DequeueHashTable* dqht) {
    if (dqht == NULL) {
        return;
    }
    ht_destroy(dqht->ht);
    free(dqht);
}

void* dqht_get(DequeueHashTable* dqht, const char* key) {
    if (dqht == NULL || key == NULL) {
        return NULL;
    }
    DQHTEntry * entry = ht_get(dqht->ht, key);
    return entry != NULL ? entry->ptr : NULL;
}

void __dqht_unlink(DequeueHashTable* dqht, DQHTEntry* entry) {
    DQHTEntry* prev_entry = entry->prev;
    DQHTEntry* next_entry = entry->next;
    if (prev_entry != NULL) {
        prev_entry->next = next_entry;
    } else {
        dqht->head = next_entry;
    }
    if (next_entry != NULL) {
        next_entry->prev = prev_entry;
    } else {
        dqht->tail = prev_entry;
    }
}

int dqht_insert(DequeueHashTable* dqht, const char* key, void* value) {
    if (dqht == NULL || dqht->ht == NULL || key == NULL) {
        return -1;
    }
    DQHTEntry* entry = ht_insert(dqht->ht, key, value);
    if (entry == NULL) {
        return -1;
    }
    if (dqht->tail != NULL) {
        dqht->tail->next = entry;
        entry->prev = dqht->tail;
    } else {
        dqht->head = entry;
    }
    dqht->tail = entry;
    return 0;
}

int dqht_remove(DequeueHashTable* dqht, const char* key) {
    if (dqht == NULL || dqht->ht == NULL || key == NULL) {
        return -1;
    }
    uint64_t hash = fnv1a_hash(key);
    size_t index = (size_t)(hash & (uint64_t)(dqht->ht->size - 1));

    for (int i = 0; i < dqht->ht->size; i++) {
        if (dqht->ht->items[index] != NULL
            && dqht->ht->items[index]->key != NULL
            && strcmp(key, dqht->ht->items[index]->key) == 0) {
            __dqht_unlink(dqht, dqht->ht->items[index]);
            dqhtentry_destroy(dqht->ht->items[index]);
            dqht->ht->items[index] = NULL;
            dqht->ht->count--;
            return 0;
        }
        index = (index + 1) % dqht->ht->size;
    }
    return -1;
}

void* dqht_get_front(DequeueHashTable* dqht) {
    return dqht != NULL ? dqht->head : NULL;
}

int dqht_push_front(DequeueHashTable* dqht, const char* key, void* value) {
    if (dqht == NULL || dqht->ht == NULL || key == NULL) {
        return -1;
    }
    DQHTEntry* entry = ht_insert(dqht->ht, key, value);
    if (entry == NULL) {
        return -1;
    }
    if (dqht->head != NULL) {
        dqht->head->prev = entry;
        entry->next = dqht->head;
    } else {
        dqht->tail = entry;
    }
    dqht->head = entry;
    return 0;
}

void* dqht_pop_front(DequeueHashTable* dqht) {
    if (dqht == NULL || dqht->head == NULL) {
        return NULL;
    }
    DQHTEntry* front = dqht->head;
    void* value = front->ptr;
    __dqht_unlink(dqht, dqht->head);
    dqhtentry_destroy(front);
    dqht->ht->items[front->index] = NULL;
    dqht->ht->count--;
    return value;
}

void* dqht_get_last(DequeueHashTable* dqht) {
    return dqht != NULL ? dqht->tail : NULL;
}

int dqht_push_last(DequeueHashTable* dqht, const char* key, void* value) {
    return dqht_insert(dqht, key, value);
}

void* dqht_pop_last(DequeueHashTable* dqht) {
    if (dqht == NULL || dqht->tail == NULL) {
        return NULL;
    }
    DQHTEntry* back = dqht->tail;
    void* value = back->ptr;
    __dqht_unlink(dqht, dqht->tail);
    dqhtentry_destroy(back);
    dqht->ht->items[back->index] = NULL;
    dqht->ht->count--;
    printf("Count: %d\n", dqht->ht->count);
    return value;
}

void dqht_print_table(DequeueHashTable* dqht) {
    for (int i = 0; i < dqht->ht->size; i++) {
        DQHTEntry * entry = dqht->ht->items[i];
        if (entry != NULL) {
            printf(
                "Entry %d: [K: %s, V: %p]\n",
                i,
                dqht->ht->items[i]->key,
                dqht->ht->items[i]->ptr
            );
            continue;
        }
        printf("Entry %d: (nil)\n", i);
    }
}