#include "dq_hashtable.h"

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "../../logging/logging.h"
#include "../hashtable/hashing.h"

DequeueHashTable* dqht_create(AM_ALLOCATOR_PARAM size_t size, KeyComparator comparator) {
    HashTable* ht = ht_create(AM_ALLOCATOR_ARG size, comparator);
    if (ht == NULL) {
        return NULL;
    }
    DequeueHashTable* dqht = am_malloc(sizeof(*dqht));
    if (dqht == NULL) {
        return NULL;
    }
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

void dqht_unlink(DequeueHashTable* dqht, DQHTEntry* entry) {
    if (dqht == NULL || entry == NULL) {
        return;
    }
    DQHTEntry* prev_entry = entry->prev;
    DQHTEntry* next_entry = entry->next;
    if (prev_entry != NULL) {
//        dump(prev_entry, sizeof(DQHTEntry));
        prev_entry->next = next_entry;
    } else {
        printf("DQHT %p\n", dqht);
        printf("dqht->head before %p\n", dqht->head);
        dqht->head = next_entry;
    }
    printf("Before next_entry check %p\n", next_entry);
    if (next_entry != NULL) {
        printf("Non NULL next_entry->prev before %p\n", next_entry->prev);
        next_entry->prev = prev_entry;
        printf("Non NULL next_entry->prev after %p\n", next_entry->prev);
    } else {
        printf("DQHT %p\n", dqht);
        printf("dqht->tail before %p\n", dqht->tail);
        dqht->tail = prev_entry;
        printf("dqht->tail after: %p\n", dqht->tail);
    }
    printf("End unlink\n");
}

int dqht_insert(AM_ALLOCATOR_PARAM DequeueHashTable* dqht, const char* key, void* value, void** overriddenValue) {
    if (dqht == NULL || key == NULL || DQHT_STRICT_CHECK(dqht)) {
        return -1;
    }
    printf("Inserting [%s: %p]\n", key, value);
    DQHTEntry* entry;
    int result;
    if ((result = ht_insert(AM_ALLOCATOR_ARG dqht->ht, key, value, &entry, overriddenValue)) == -1) {
        return -1;
    } else if (result == 0) {
        // Updated an existing entry, no need set links
        return 0;
    }
    if (dqht->tail != NULL) {
        printf("Amending tail: [%s: %p]\n", entry->key, entry->ptr);
        dqht->tail->next = entry;
        printf("Entry->prev before %p\n", entry->prev);
        entry->prev = dqht->tail;
        printf("Entry->prev after: %p\n", entry->prev);
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
    size_t hash = hash_key(key);
    size_t index = (size_t)(hash & (size_t)(dqht->ht->size - 1));

    for (int i = 0; i < dqht->ht->size; i++) {
        if (dqht->ht->items[index] != NULL
            && dqht->ht->items[index]->key != NULL
            && strcmp(key, dqht->ht->items[index]->key) == 0) {
            dqht_print_table("Before unlink", dqht);
            dqht_unlink(dqht, dqht->ht->items[index]);
            dqht_print_table("After unlink", dqht);
            return ht_delete_entry(AM_ALLOCATOR_ARG dqht->ht, index);
        }
        index = (index + 1) % dqht->ht->size;
    }
    return NULL;
}

void* dqht_get_front(DequeueHashTable* dqht) {
    return dqht != NULL || DQHT_STRICT_CHECK(dqht) ? dqht->head : NULL;
}

int dqht_push_front(AM_ALLOCATOR_PARAM DequeueHashTable* dqht, const char* key, void* value, void** overriddenValue) {
    if (dqht == NULL || key == NULL || DQHT_STRICT_CHECK(dqht)) {
        return -1;
    }
    DQHTEntry* entry;
    int result;
    if ((result = ht_insert(AM_ALLOCATOR_ARG dqht->ht, key, value, &entry, overriddenValue)) == -1) {
        return -1;
    } else if (result == 0) {
        // Updated an existing entry, nothing to do
        return 0;
    }
    if (dqht->head != NULL) {
        printf("dqht->head->prev before %p\n", dqht->head->prev);
        dqht->head->prev = entry;
        printf("dqht->head->prev after %p\n", dqht->head->prev);
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
    printf("Front %p\n", front);
    dqht_print_table("PRE FRONT", dqht);
    dqht_unlink(dqht, front);
    dqht_print_table("POST FRONT", dqht);
    return ht_delete_entry(AM_ALLOCATOR_ARG dqht->ht, front->index);
}

void* dqht_get_last(DequeueHashTable* dqht) {
    return dqht != NULL || DQHT_STRICT_CHECK(dqht) ? dqht->tail : NULL;
}

int dqht_push_last(AM_ALLOCATOR_PARAM DequeueHashTable* dqht, const char* key, void* value) {
    return dqht_insert(AM_ALLOCATOR_ARG dqht, key, value, NULL);
}

void* dqht_pop_last(AM_ALLOCATOR_PARAM DequeueHashTable* dqht) {
    if (dqht == NULL || dqht->tail == NULL || DQHT_STRICT_CHECK(dqht)) {
        return NULL;
    }
    DQHTEntry* back = dqht->tail;
    printf("Back %p\n", back);
    dqht_unlink(dqht, back);
    return ht_delete_entry(AM_ALLOCATOR_ARG dqht->ht, back->index);
}

void dqht_print_table(char* name, DequeueHashTable* dqht) {
    if (dqht == NULL || DQHT_STRICT_CHECK(dqht)) {
        return;
    }
    INFO("TABLE: %s", name);
    if (dqht->head != NULL) {
        printf(
            "[Head: [%zu] %s:%p (N: %s %p, P: %s %p)]\n",
            dqht->head->index,
            dqht->head->key,
            dqht->head->ptr,
            dqht->head->next == NULL ? "<None>" : dqht->head->next->key,
            dqht->head->next == NULL ? NULL : dqht->head->next->ptr,
            dqht->head->prev == NULL ? "<None>" : dqht->head->prev->key,
            dqht->head->prev == NULL ? NULL : dqht->head->prev->ptr
        );
    } else {
        printf("[Head: (nil)]\n");
    }
    if (dqht->tail != NULL) {
        printf(
            "[Tail: [%zu] %s:%p (N: %s %p, P: %s %p)]\n",
            dqht->tail->index,
            dqht->tail->key,
            dqht->tail->ptr,
            dqht->tail->next == NULL ? "<None>" : dqht->tail->next->key,
            dqht->tail->next == NULL ? NULL : dqht->tail->next->ptr,
            dqht->tail->prev == NULL ? "<None>" : dqht->tail->prev->key,
            dqht->tail->prev == NULL ? NULL : dqht->tail->prev->ptr
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
            entry->key,
            entry->ptr,
            entry->next == NULL ? "<None>" : entry->next->key,
            entry->next == NULL ? NULL : entry->next->ptr,
            entry->prev == NULL ? "<None>" : entry->prev->key,
            entry->prev == NULL ? NULL : entry->prev->ptr,
            entry->next != NULL ? "," : " "
        );
        entry = entry->next;
    }
    printf(" }\n");
}