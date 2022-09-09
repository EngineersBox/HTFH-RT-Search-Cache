#include "cache_hashtable.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "hashing.h"
#include "../../logging/logging.h"

HashTable* ht_create(AM_ALLOCATOR_PARAM size_t size, KeyComparator comparator) {
    HashTable* ht = am_malloc(sizeof(*ht));
    ht->size = size;
    ht->count = 0;
    ht->comparator = comparator == NULL ? strcmp : comparator;
    ht->items = am_calloc(ht->size, sizeof(DQHTEntry*));
    return ht;
}

void ht_destroy(AM_ALLOCATOR_PARAM HashTable* ht) {
    if (ht == NULL || ht->items == NULL) {
        return;
    }
    am_free(ht->items);
    am_free(ht);
}

int ht_insert(AM_ALLOCATOR_PARAM HashTable* ht, const char* key, void* value, DQHTEntry** entry, void** overriddenValue) {
    if (ht == NULL
        || ht->items == NULL
        || value == NULL
        || ht->count >= (ht->size / 2) && ht_resize(AM_ALLOCATOR_ARG ht) != 0) {
        return -1;
    }
    size_t hash = hash_key(key);
    size_t index = (size_t)(hash % ((size_t)(ht->size - 1)));

    while(ht->items[index] != NULL) {
        if (ht->items[index]->key != NULL && strcmp(key, ht->items[index]->key) == 0) {
            if (overriddenValue != NULL && ht->items[index]->ptr != value) {
                *overriddenValue = ht->items[index]->ptr;
                INFO("[CACHE HASHTABLE] overridden value set: %p", *overriddenValue);
            }
            ht->items[index]->ptr = value;
            INFO("[CACHE HASHTABLE] New value: %p, Overridden value: %p", ht->items[index]->ptr, overriddenValue != NULL ? *overriddenValue : NULL);
            if (entry != NULL) {
                *entry = ht->items[index];
            }
            return 0;
        }
        index = (index + 1) % ht->size;
    }
    ht->items[index] = dqhtentry_create(AM_ALLOCATOR_ARG key, value);
    if (ht->items[index] == NULL) {
        return -1;
    }
    ht->items[index]->index = index;
    if (entry != NULL) {
        *entry = ht->items[index];
    }
    ht->count++;
    return 1;
}

int ht_resize_insert(DQHTEntry** items, size_t size, DQHTEntry* entry, size_t index) {
    while(items[index] != NULL) {
        if (strcmp(entry->key, items[index]->key) == 0) {
            return -1;
        }
        index = index + 1 % size;
    }
    items[index] = entry;
    items[index]->index = index;
    return 0;
}

void print_table(HashTable* ht) {
    for (int i = 0; i < ht->size; i++) {
        printf("Entry %d: %p\n", i, ht->items[i]);
    }
}

int ht_resize(AM_ALLOCATOR_PARAM HashTable* ht) {
    size_t new_size = ht->size * 2;
    if (new_size < ht->size) {
        return -1;
    }
    DQHTEntry** new_items = am_calloc(new_size, sizeof(DQHTEntry*));
    if (new_items == NULL) {
        return -1;
    }
    size_t hash;
    size_t index;
    for (int i = 0; i < ht->size; i++) {
        if (ht->items[i] == NULL) {
            continue;
        }
        hash = hash_key(ht->items[i]->key);
        index = (size_t)(hash % ((uint64_t)(new_size - 1)));
        if (ht_resize_insert(new_items, new_size, ht->items[i], index) != 0) {
            return -1;
        }
    }
    am_free(ht->items);
    ht->items = new_items;
    ht->size = new_size;
    return 0;
}

DQHTEntry* ht_get(HashTable* ht, const char* key) {
    if (ht == NULL
        || ht->items == NULL
        || key == NULL) {
        return NULL;
    }
    size_t hash = hash_key(key);
    size_t index = (size_t)(hash & (size_t)(ht->size - 1));

    for (int i = 0; i < ht->size; i++) {
        if (ht->items[index] != NULL
            && ht->items[index]->key != NULL
            && strcmp(key, ht->items[index]->key) == 0) {
            return ht->items[index];
        }
        index = (index + 1) % ht->size;
    }
    return NULL;
}

void* ht_delete_entry(AM_ALLOCATOR_PARAM HashTable* ht, int index) {
    if (ht == NULL || ht->items == NULL || ht->items[index] == NULL) {
        return NULL;
    }
    void* value = ht->items[index]->ptr;
    dqhtentry_destroy(AM_ALLOCATOR_ARG ht->items[index]);
    ht->count--;
    ht->items[index] = NULL;
    return value;
}

void* ht_delete(AM_ALLOCATOR_PARAM HashTable* ht, const char* key) {
    if (ht == NULL
        || ht->items == NULL
        || key == NULL) {
        return NULL;
    }
    size_t hash = hash_key(key);
    size_t index = (size_t)(hash & (size_t)(ht->size - 1));

    for (int i = 0; i < ht->size; i++) {
        if (ht->items[index] != NULL
            && ht->items[index]->key != NULL
            && strcmp(key, ht->items[index]->key) == 0) {
            return ht_delete_entry(AM_ALLOCATOR_ARG ht, index);
        }
        index = (index + 1) % ht->size;
    }
    return NULL;
}