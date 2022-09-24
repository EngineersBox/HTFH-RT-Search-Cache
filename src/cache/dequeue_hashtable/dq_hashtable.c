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
    TRACE("Getting current");
    DQHTEntry* current = dqht->head;
    TRACE("Current: %p", current);
    DQHTEntry* next = current == NULL ? NULL : dqht->head->next;
    TRACE("Next: %p", next);
    while (current != NULL) {
        if (handler != NULL) {
            TRACE("Invoking handler: %p", handler);
            TRACE("Handling current ptr: %p", current->ptr);
            handler(AM_ALLOCATOR_ARG current->ptr, callbackState);
            current->ptr = NULL;
            TRACE("Nullified current ptr: %p", current->ptr);
        }
        TRACE("Destroying current: %p", current);
        dqhtentry_destroy(AM_ALLOCATOR_ARG current);
        TRACE("Destroyed current: %p", current);
        current = next;
        TRACE("Moved next to current");
        if (next != NULL) {
            next = next->next;
            TRACE("Next 2: %p", next);
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
    dqht_print_table("[DQHT] Unlink before: ", dqht);
//    printf("Unlink entry: %p\n", entry);
//    printf("Unlink prev_entry check %p\n", prev_entry);
    if (prev_entry != NULL) {
//        printf("Next entry: %p\n", next_entry);
        prev_entry->next = next_entry;
//        printf("Non NULL prev_entry->next after %p\n", prev_entry->next);
    } else {
//        printf("DQHT %p\n", dqht);
//        printf("dqht->head before %p\n", dqht->head);
        dqht->head = next_entry;
//        printf("dqht->head after %p\n", dqht->head);
    }
//    printf("Before next_entry check %p\n", next_entry);
    if (next_entry != NULL) {
//        printf("Non NULL next_entry->prev before %p\n", next_entry->prev);
        next_entry->prev = prev_entry;
//        printf("Non NULL next_entry->prev after %p\n", next_entry->prev);
    } else {
//        printf("DQHT %p\n", dqht);
//        printf("dqht->tail before %p\n", dqht->tail);
        dqht->tail = prev_entry;
//        printf("dqht->tail after: %p\n", dqht->tail);
    }
    dqht_print_table("[DQHT] Unlink after: ", dqht);
//    printf("End unlink\n");
}

int dqht_insert(AM_ALLOCATOR_PARAM DequeueHashTable* dqht, const char* key, void* value) {
    if (dqht == NULL || key == NULL || DQHT_STRICT_CHECK(dqht)) {
        return -1;
    }
//    printf("Inserting [%s: %p]\n", key, value);
    DQHTEntry* entry = ht_insert(AM_ALLOCATOR_ARG dqht->ht, key, value);
    if (entry == NULL) {
        return -1;
    }
    if (dqht->tail != NULL) {
//        printf("Amending tail: [%s: %p]\n", key_sprint(entry->key), entry->ptr);
        dqht->tail->next = entry;
//        printf("Entry->prev before %p\n", entry->prev);
        entry->prev = dqht->tail;
//        printf("Entry->prev after: %p\n", entry->prev);
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
        printf("[DQHT] Removing: %s %zu\n", key_sprint(key), index);
        if (dqht->ht->items[index] != NULL
            && dqht->ht->items[index]->key != NULL
            && key_cmp(key, dqht->ht->items[index]->key) == 0) {
//            dqht_print_table("Before unlink", dqht);
            dqht_unlink(dqht, dqht->ht->items[index]);
//            dqht_print_table("After unlink", dqht);
            return ht_delete_entry(AM_ALLOCATOR_ARG dqht->ht, index);
        }
        index = (index + 1) % dqht->ht->size;
    }
    return NULL;
}

void* dqht_get_front(DequeueHashTable* dqht) {
    return dqht == NULL || DQHT_STRICT_CHECK(dqht) ? NULL : dqht->head;
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
//        printf("dqht->head->prev before %p\n", dqht->head->prev);
        dqht->head->prev = entry;
//        printf("dqht->head->prev after %p\n", dqht->head->prev);
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
    printf("Front %p %zu\n", front, front->index);
    dqht_print_table("PRE FRONT", dqht);
    dqht_unlink(dqht, front);
    dqht_print_table("POST FRONT", dqht);
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
//    printf("Back %p\n", back);
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