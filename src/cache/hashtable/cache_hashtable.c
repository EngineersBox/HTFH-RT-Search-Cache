#include "cache_hashtable.h"

#include <stdlib.h>
#include "hashing.h"

HashTable* ht_create(size_t size) {
    HashTable* ht = malloc(sizeof(*ht));
    ht->size = size;
    ht->count = 0;
    ht->items = calloc(ht->size, sizeof(HTEntry*));
    return ht;
}

void ht_destroy(HashTable* ht) {
    if (ht == NULL) {
        return;
    }
    for (int i = 0; i < ht->size; i++) {
        HTEntry* entry = ht->items[i];
        if (entry != NULL) {
            htentry_destroy(entry);
        }
    }
    free(ht->items);
    free(ht);
}

void* ht_insert(HashTable* ht, const char* key, void* value) {
    if (ht == NULL) {
        return NULL;
    } else if (value == NULL) {
        return NULL;
    } else if (value == NULL) {
        return NULL;
    } else if (ht->count >= (ht->size / 2) && ht_resize(ht) != 0) {
        return NULL;
    }
    uint64_t hash = fnv1a_hash(key);
    size_t index = (size_t)(hash % ((uint64_t)(ht->size - 1)));

    while(ht->items[index] != NULL) {
        if (strcmp(key, ht->items[index]->key) == 0) {
            void* current_value = ht->items[index]->ptr;
            ht->items[index]->ptr = value;
            return current_value;
        }
        index = (index + 1) % ht->size;
    }
    ht->items[index] = htentry_create(key, value);
    if (ht->items[index] == NULL) {
        return NULL;
    }
    ht->count++;
    return value;
}

inline int ht_resize_insert(HTEntry** items, size_t size, HTEntry* entry, size_t index) {
    while(items[index] != NULL) {
        if (strcmp(entry->key, items[index]->key) == 0) {
            return -1;
        }
        index = index + 1 % size;
    }
    items[index] = entry;
    return 0;
}

#include <stdio.h>

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
    HTEntry** new_items = calloc(new_size, sizeof(HTEntry*));
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

void* ht_get(HashTable* ht, const char* key) {
    if (ht == NULL) {
        return NULL;
    } else if (key == NULL) {
        return NULL;
    }
    uint64_t hash = fnv1a_hash(key);
    size_t index = (size_t)(hash & (uint64_t)(ht->size - 1));

    for (int i = 0; i < ht->size; i++) {
        if (ht->items[index] != NULL && strcmp(key, ht->items[index]->key) == 0) {
            return ht->items[index]->ptr;
        }
        index = (index + 1) % ht->size;
    }
    return NULL;
}

void* ht_delete(HashTable* ht, const char* key) {
    if (ht == NULL) {
        return NULL;
    } else if (key == NULL) {
        return NULL;
    }
    uint64_t hash = fnv1a_hash(key);
    size_t index = (size_t)(hash & (uint64_t)(ht->size - 1));

    for (int i = 0; i < ht->size; i++) {
        if (ht->items[index] != NULL && strcmp(key, ht->items[index]->key) == 0) {
            void* current_value = ht->items[index]->ptr;
            htentry_destroy(ht->items[index]);
            return current_value;
        }
        index = (index + 1) % ht->size;
    }
    return NULL;
}