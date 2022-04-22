#include "dq_hashtable.h"

#include <stdlib.h>
#include "hashing.h"

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
        return NULL;
    }
    ht_destroy(dqht->ht);
    free(dqht);
}

void* dqht_get(DequeueHashTable* dqht, const char* key) {
    if (dqht == NULL || key == NULL) {
        return NULL;
    }
    return ht_get(dqht->ht, key);
}

void __dqht_unlink(DequeueHashTable* dqht, DQHTEntry* entry) {
    DQHTEntry* prev_entry = entry->prev;
    DQHTEntry* next_entry = entry->next;
    if (prev_entry != NULL) {
        prev_entry->next = next_entry;
    } else {
        dqht->tail = next_entry;
    }
    if (next_entry != NULL) {
        next_entry->prev = prev_entry;
    } else {
        dqht->head = prev_entry;
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
    __dqht_unlink(dqht, entry);
    DQHTEntry* last = dqht->tail;
    if (last != NULL) {
        last->next = entry;
        entry->prev = last;
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
    DQHTEntry* entry = ht_delete(dqht->ht, key);
    if (entry == NULL) {
        return -1;
    }
    __dqht_unlink(dqht, entry);
    return 0;
}

void* dqht_get_front(DequeueHashTable* dqht) {
    return dqht != NULL ? dqht->head : NULL;
}

int dqht_push_front(DequeueHashTable* dqht, const char* key, void* value) {
    // TODO: Refactor so that pushing key matching existing element moves element to front
    if (dqht == NULL || dqht->ht == NULL || key == NULL) {
        return -1;
    }
    DQHTEntry* entry = ht_insert(dqht->ht, key, value);
    if (entry == NULL) {
        return -1;
    }
    __dqht_unlink(dqht, entry);
    DQHTEntry* first = dqht->head;
    if (first != NULL) {
        first->prev = entry;
        entry->next = first;
    } else {
        dqht->tail = entry;
    }
    dqht->head = entry;
    return 0;
}

void* dqht_pop_front(DequeueHashTable* dqht) {
    if (dqht == NULL) {
        return NULL;
    }
    DQHTEntry* front = dqht->head;
    if (front == NULL) {
        return NULL;
    }
    if (front->prev != NULL) {
        front->prev->next = NULL;
        front->prev = NULL;
    }
    void* value = front->ptr;
    dqhtentry_destroy(front);
    return value;
}

void* dqht_get_last(DequeueHashTable* dqht) {
    return dqht != NULL ? dqht->tail : NULL;
}

int dqht_push_last(DequeueHashTable* dqht, const char* key, void* value) {
    return dqht_insert(dqht, key, value);
}

void* dqht_pop_last(DequeueHashTable* dqht) {
    if (dqht == NULL) {
        return NULL;
    }
    DQHTEntry* back = dqht->tail;
    if (back == NULL) {
        return NULL;
    }
    if (back->next != NULL) {
        back->next->prev = NULL;
        back->next = NULL;
    }
    void* value = back->ptr;
    dqhtentry_destroy(back);
    return value;
}

#include <stdio.h>

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
        printf("Entry %d: %p\n", i, entry);
    }
}