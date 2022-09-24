#include "dq_hashtable.h"

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "../../logging/logging.h"
#include "../hashtable/hashing.h"
#include "../cache_key.h"
#include "../dlirs/dlirs_entry.h"

DequeueHashTable* dqht_create(AM_ALLOCATOR_PARAM size_t size, KeyComparator comparator) {
    HashTable* ht = ht_create(AM_ALLOCATOR_ARG size, comparator);
    if (ht == NULL) {
        return NULL;
    }
    DequeueHashTable* dqht = (DequeueHashTable*) am_malloc(sizeof(*dqht));
    if (dqht == NULL) {
        return NULL;
    }
    dqht->keyComparator = comparator;
    dqht->head = NULL;
    dqht->tail = NULL;
    dqht->ht = ht;
    return dqht;
}

void dqht_destroy_handled(AM_ALLOCATOR_PARAM DequeueHashTable* dqht, EntryValueDestroyHandler handler, void* callbackState) {
    if (dqht == NULL || DQHT_STRICT_CHECK(dqht)) {
        return;
    }
    DQHTEntry* current = dqht->head;
    DQHTEntry* next = current == NULL ? NULL : dqht->head->next;
    while (current != NULL) {
        if (handler != NULL) {
            handler(AM_ALLOCATOR_ARG current->ptr, callbackState);
            current->ptr = NULL;
        }
        current = next;
        if (next != NULL) {
            next = next->next;
        }
    }
    ht_destroy(AM_ALLOCATOR_ARG dqht->ht);
    am_free(dqht);
}

void dqht_destroy(AM_ALLOCATOR_PARAM DequeueHashTable* dqht) {
    dqht_destroy_handled(AM_ALLOCATOR_ARG dqht, NULL, NULL);
}

void* dqht_get(DequeueHashTable* dqht, const char* key) {
    if (dqht == NULL || key == NULL || DQHT_STRICT_CHECK(dqht)) {
        return NULL;
    }
    DQHTEntry* entry = ht_get(dqht->ht, key);
    return entry != NULL ? entry->ptr : NULL;
}

void* dqht_get_custom(DequeueHashTable* dqht, const char* key) {
    if (dqht == NULL || key == NULL || DQHT_STRICT_CHECK(dqht)) {
        return NULL;
    }
    DQHTEntry* entry = ht_get_custom(dqht->ht, key);
    return entry != NULL ? entry->ptr : NULL;
}

void dqht_unlink(DequeueHashTable* dqht, DQHTEntry* entry) {
    if (dqht == NULL || entry == NULL) {
        return;
    }
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

int dqht_insert(AM_ALLOCATOR_PARAM DequeueHashTable* dqht, const char* key, void* value) {
    if (dqht == NULL || key == NULL || DQHT_STRICT_CHECK(dqht)) {
        return -1;
    }
    DQHTEntry* entry = ht_insert(AM_ALLOCATOR_ARG dqht->ht, key, value);
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

void* dqht_remove(AM_ALLOCATOR_PARAM DequeueHashTable* dqht, const char* key) {
    if (dqht == NULL || key == NULL || DQHT_STRICT_CHECK(dqht)) {
        return NULL;
    }
#if defined(HASH_FUNC) && HASH_FUNC == MEIYAN
    size_t hash = meiyan_hash(key);
#else
    size_t hash = fnv1a_hash(key);
#endif
    size_t index = hash % dqht->ht->size;
    for (int i = 0; i < dqht->ht->size; i++) {
        if (dqht->ht->items[index] != NULL
            && dqht->ht->items[index]->key != NULL
            && key_cmp(key, dqht->ht->items[index]->key) == 0) {
            dqht_unlink(dqht, dqht->ht->items[index]);
            return ht_delete_entry(AM_ALLOCATOR_ARG dqht->ht, index);
        }
        index = (index + 1) % dqht->ht->size;
    }
    return NULL;
}

void* dqht_get_front(DequeueHashTable* dqht) {
    return dqht == NULL || DQHT_STRICT_CHECK(dqht) ? NULL : dqht->head->ptr;
}

int dqht_push_front(AM_ALLOCATOR_PARAM DequeueHashTable* dqht, const char* key, void* value) {
    if (dqht == NULL || key == NULL || DQHT_STRICT_CHECK(dqht)) {
        return -1;
    }
    DQHTEntry* entry = ht_insert(AM_ALLOCATOR_ARG dqht->ht, key, value);
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

void* dqht_pop_front(AM_ALLOCATOR_PARAM DequeueHashTable* dqht) {
    if (dqht == NULL || dqht->head == NULL || DQHT_STRICT_CHECK(dqht)) {
        return NULL;
    }
    DQHTEntry* front = dqht->head;
    dqht_unlink(dqht, front);
    return ht_delete_entry(AM_ALLOCATOR_ARG dqht->ht, front->index);
}

void* dqht_get_last(DequeueHashTable* dqht) {
    return dqht != NULL || DQHT_STRICT_CHECK(dqht) ? dqht->tail : NULL;
}

int dqht_push_last(AM_ALLOCATOR_PARAM DequeueHashTable* dqht, const char* key, void* value) {
    return dqht_insert(AM_ALLOCATOR_ARG dqht, key, value);
}

void* dqht_pop_last(AM_ALLOCATOR_PARAM DequeueHashTable* dqht) {
    if (dqht == NULL || dqht->tail == NULL || DQHT_STRICT_CHECK(dqht)) {
        return NULL;
    }
    DQHTEntry* back = dqht->tail;
    dqht_unlink(dqht, back);
    return ht_delete_entry(AM_ALLOCATOR_ARG dqht->ht, back->index);
}

void dqht_print_table(char* name, DequeueHashTable* dqht) {
    if (dqht == NULL || DQHT_STRICT_CHECK(dqht)) {
        return;
    }
    printf("TABLE: %s\n", name);
    if (dqht->head != NULL) {
        printf(
            "[Head: [%zu] %s:%p]\n",
            dqht->head->index,
            key_sprint(dqht->head->key),
            dqht->head->ptr
        );
    } else {
        printf("[Head: (nil)]\n");
    }
    if (dqht->tail != NULL) {
        printf(
            "[Tail: [%zu] %s:%p]\n",
            dqht->tail->index,
            key_sprint(dqht->tail->key),
            dqht->tail->ptr
        );
    } else {
        printf("[Tail: (nil)]\n");
    }
    printf("{");
    DQHTEntry* entry = dqht->head;
    while (entry != NULL) {
        printf(
            " [%zu] %s: %p (N: %s %p, P: %s %p)%s",
            entry->index,
            key_sprint(entry->key),
            entry->ptr,
            entry->next == NULL ? "" : key_sprint(entry->next->key),
            entry->next == NULL ? NULL : entry->next->ptr,
            entry->prev == NULL ? "" : key_sprint(entry->prev->key),
            entry->prev == NULL ? NULL : entry->prev->ptr,
            entry->next != NULL ? "," : " "
        );
        entry = entry->next;
    }
    printf(" }\n");
}