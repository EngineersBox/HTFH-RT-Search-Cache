#include "dq_hashtable.h"

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#define ENABLE_LOGGING
#define LOG_DATETIME_PREFIX
#include "../logging/logging.h"
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

void dqht_destroy_handled(DequeueHashTable* dqht, EntryValueDestroyHandler handler, void* callbackState) {
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
            handler(current->ptr, callbackState);
            current->ptr = NULL;
            TRACE("Nullified current ptr: %p", current->ptr);
        }
        TRACE("Destroying current: %p", current);
        dqhtentry_destroy(current);
        TRACE("Destroyed current: %p", current);
        current = next;
        TRACE("Moved next to current");
        if (next != NULL) {
            next = next->next;
            TRACE("Next 2: %p", next);
        }
    }
    ht_destroy(dqht->ht);
    free(dqht);
}

void dqht_destroy(DequeueHashTable* dqht) {
    dqht_destroy_handled(dqht, NULL, NULL);
}

void* dqht_get(DequeueHashTable* dqht, const char* key) {
    if (dqht == NULL || key == NULL || DQHT_STRICT_CHECK(dqht)) {
        return NULL;
    }
    DQHTEntry* entry = ht_get(dqht->ht, key);
    return entry != NULL ? entry->ptr : NULL;
}

void dqht_unlink(DequeueHashTable* dqht, DQHTEntry* entry) {
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
    if (dqht == NULL || key == NULL || DQHT_STRICT_CHECK(dqht)) {
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

void* dqht_remove(DequeueHashTable* dqht, const char* key) {
    if (dqht == NULL || key == NULL || DQHT_STRICT_CHECK(dqht)) {
        return NULL;
    }
    uint64_t hash = fnv1a_hash(key);
    size_t index = (size_t)(hash & (uint64_t)(dqht->ht->size - 1));

    for (int i = 0; i < dqht->ht->size; i++) {
        if (dqht->ht->items[index] != NULL
            && dqht->ht->items[index]->key != NULL
            && strcmp(key, dqht->ht->items[index]->key) == 0) {
            dqht_print_table("Before unlink", dqht);
            dqht_unlink(dqht, dqht->ht->items[index]);
            dqht_print_table("After unlink", dqht);
            void* value = dqht->ht->items[index]->ptr;
            dqhtentry_destroy(dqht->ht->items[index]);
            dqht->ht->items[index] = NULL;
            dqht->ht->count--;
            return value;
        }
        index = (index + 1) % dqht->ht->size;
    }
    return NULL;
}

void* dqht_get_front(DequeueHashTable* dqht) {
    return dqht != NULL || DQHT_STRICT_CHECK(dqht) ? dqht->head : NULL;
}

int dqht_push_front(DequeueHashTable* dqht, const char* key, void* value) {
    if (dqht == NULL || key == NULL || DQHT_STRICT_CHECK(dqht)) {
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
    if (dqht == NULL || dqht->head == NULL || DQHT_STRICT_CHECK(dqht)) {
        return NULL;
    }
    DQHTEntry* front = dqht->head;
    void* value = front->ptr;
    dqht_unlink(dqht, dqht->head);
    dqht->ht->items[front->index] = NULL;
    dqhtentry_destroy(front);
    dqht->ht->count--;
    return value;
}

void* dqht_get_last(DequeueHashTable* dqht) {
    return dqht != NULL || DQHT_STRICT_CHECK(dqht) ? dqht->tail : NULL;
}

int dqht_push_last(DequeueHashTable* dqht, const char* key, void* value) {
    return dqht_insert(dqht, key, value);
}

void* dqht_pop_last(DequeueHashTable* dqht) {
    if (dqht == NULL || dqht->tail == NULL || DQHT_STRICT_CHECK(dqht)) {
        return NULL;
    }
    DQHTEntry* back = dqht->tail;
    void* value = back->ptr;
    dqht_unlink(dqht, dqht->tail);
    dqht->ht->items[back->index] = NULL;
    dqhtentry_destroy(back);
    dqht->ht->count--;
    return value;
}

void dqht_print_table(char* name, DequeueHashTable* dqht) {
    if (dqht == NULL || DQHT_STRICT_CHECK(dqht)) {
        return;
    }
    char printString[2048] = "";
    strcat(printString, name);
    char headElement[100];
    if (dqht->head != NULL) {
        sprintf(
            headElement,
            " [Head: [%lld] %s:%p]",
            dqht->head->index,
            dqht->head->key,
            dqht->head->ptr
        );
    } else {
        strcpy(headElement, " [Head: (nil)]");
    }
    strcat(printString, headElement);
    char tailElement[100];
    if (dqht->tail != NULL) {
        sprintf(
            tailElement,
            " [Tail: [%lld] %s:%p]",
            dqht->tail->index,
            dqht->tail->key,
            dqht->tail->ptr
        );
    } else {
        strcpy(tailElement, " [Tail: (nil)]");
    }
    strcat(printString, tailElement);
    strcat(printString, " {");
    DQHTEntry* entry = dqht->head;
    while (entry != NULL) {
        char formatString[100];
        sprintf(
            formatString,
            " [%zu] %s: %p%s",
            entry->index,
            entry->key,
            entry->ptr,
            entry->next != NULL ? "," : " "
        );
        strcat(printString, formatString);
        entry = entry->next;
    }
    strcat(printString, "}");
    INFO("%s", printString);
}