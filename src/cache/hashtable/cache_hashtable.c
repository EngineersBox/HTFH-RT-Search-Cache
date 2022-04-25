#include "cache_hashtable.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "hashing.h"

HashTable* ht_create(size_t size) {
    HashTable* ht = malloc(sizeof(*ht));
    ht->size = size;
    ht->count = 0;
    ht->items = calloc(ht->size, sizeof(DQHTEntry*));
    return ht;
}

void ht_destroy(HashTable* ht) {
    if (ht == NULL || ht->items == NULL) {
        return;
    }
    free(ht->items);
    free(ht);
}

DQHTEntry* ht_insert(HashTable* ht, const char* key, void* value) {
    if (ht == NULL
        || ht->items == NULL
        || value == NULL
        || ht->count >= (ht->size / 2) && ht_resize(ht) != 0) {
        return NULL;
    }
    uint64_t hash = fnv1a_hash(key);
    size_t index = (size_t)(hash % ((uint64_t)(ht->size - 1)));

    while(ht->items[index] != NULL) {
        if (ht->items[index]->key != NULL && strcmp(key, ht->items[index]->key) == 0) {
            ht->items[index]->ptr = value;
            return ht->items[index];
        }
        index = (index + 1) % ht->size;
    }
    ht->items[index] = dqhtentry_create(key, value);
    if (ht->items[index] == NULL) {
        return NULL;
    }
    ht->items[index]->index = index;
    ht->count++;
    return ht->items[index];
}

inline int ht_resize_insert(DQHTEntry** items, size_t size, DQHTEntry* entry, size_t index) {
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

int ht_resize(HashTable* ht) {
    size_t new_size = ht->size * 2;
    if (new_size < ht->size) {
        return -1;
    }
    DQHTEntry** new_items = calloc(new_size, sizeof(DQHTEntry*));
    if (new_items == NULL) {
        return -1;
    }
    uint64_t hash;
    size_t index;
    for (int i = 0; i < ht->size; i++) {
        if (ht->items[i] == NULL) {
            continue;
        }
        hash = fnv1a_hash(ht->items[i]->key);
        index = (size_t)(hash % ((uint64_t)(new_size - 1)));
        if (ht_resize_insert(new_items, new_size, ht->items[i], index) != 0) {
            return -1;
        }
    }
    free(ht->items);
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
    uint64_t hash = fnv1a_hash(key);
    size_t index = (size_t)(hash & (uint64_t)(ht->size - 1));

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

DQHTEntry* ht_delete(HashTable* ht, const char* key) {
    if (ht == NULL
        || ht->items == NULL
        || key == NULL) {
        return NULL;
    }
    uint64_t hash = fnv1a_hash(key);
    size_t index = (size_t)(hash & (uint64_t)(ht->size - 1));

    for (int i = 0; i < ht->size; i++) {
        if (ht->items[index] != NULL
            && ht->items[index]->key != NULL
            && strcmp(key, ht->items[index]->key) == 0) {
            dqhtentry_destroy(ht->items[index]);
            ht->count--;
            DQHTEntry* entry = ht->items[index];
            ht->items[index] = NULL;
            return entry;
        }
        index = (index + 1) % ht->size;
    }
    return NULL;
}