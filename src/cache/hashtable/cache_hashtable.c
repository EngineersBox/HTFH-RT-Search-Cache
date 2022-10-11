#include "cache_hashtable.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "hashing.h"
#include "../cache_key.h"
#include "../../logging/logging.h"

HashTable* ht_create(AM_ALLOCATOR_PARAM size_t size, KeyComparator comparator) {
    HashTable* ht = (HashTable*) am_malloc(sizeof(*ht));
    ht->size = size;
    ht->count = 0;
    ht->comparator = comparator;
    ht->items = (DQHTEntry**) am_calloc(ht->size, sizeof(DQHTEntry*));
    return ht;
}

void ht_destroy(AM_ALLOCATOR_PARAM HashTable* ht) {
    if (ht == NULL || ht->items == NULL) {
        return;
    }
    for (int i = 0; i < ht->size; i++) {
        if (ht->items[i] == NULL) {
            continue;
        }
        dqhtentry_destroy(AM_ALLOCATOR_ARG ht->items[i]);
    }
    am_free(ht->items);
    am_free(ht);
}

DQHTEntry* ht_insert(AM_ALLOCATOR_PARAM HashTable* ht, const char* key, void* value) {
    if (ht == NULL
        || ht->items == NULL
        || value == NULL) {
        return NULL;
    } else if (ht->count >= (ht->size / 2)) {
        if (ht_resize(AM_ALLOCATOR_ARG ht) != 0) {
            return NULL;
        }
    }
#if defined(HASH_FUNC) && HASH_FUNC == MEIYAN
    size_t hash = meiyan_hash(key);
#else
    size_t hash = fnv1a_hash(key);
#endif
    size_t index = hash % ht->size;

    for (int i = 0; i < ht->size && ht->items[index] != NULL; i++) {
        if (ht->items[index]->key != NULL
            && key_cmp(key, ht->items[index]->key) == 0) {
            ht->items[index]->ptr = value;
            return ht->items[index];
        }
        index = (index + 1) % ht->size;
    }
//    while(ht->items[index] != NULL) {
//        if (ht->items[index]->key != NULL
//            && key_cmp(key, ht->items[index]->key) == 0) {
//            ht->items[index]->ptr = value;
//            return ht->items[index];
//        }
//        index = (index + 1) % ht->size;
//    }
    ht->items[index] = dqhtentry_create(AM_ALLOCATOR_ARG key, value);
    if (ht->items[index] == NULL) {
        return NULL;
    }
    ht->items[index]->index = index;
    ht->count++;
    return ht->items[index];
}

int ht_resize_insert(DQHTEntry** items, size_t size, DQHTEntry* entry, size_t index) {
    while(items[index] != NULL) {
        if (key_cmp(entry->key, items[index]->key) == 0) {
            return -1;
        }
        index = (index + 1) % size;
    }
    items[index] = entry;
    items[index]->index = index;
    return 0;
}

void print_table(HashTable* ht) {
    for (int i = 0; i < ht->size; i++) {
        DEBUG("Entry %d: %p", i, ht->items[i]);
    }
}

int ht_resize(AM_ALLOCATOR_PARAM HashTable* ht) {
    size_t new_size = ht->size * 2;
    if (new_size < ht->size) {
        return -1;
    }
    DQHTEntry** new_items = (DQHTEntry**) am_calloc(new_size, sizeof(DQHTEntry*));
    if (new_items == NULL) {
        return -1;
    }
    size_t hash;
    size_t index;
    for (int i = 0; i < ht->size; i++) {
        if (ht->items[i] == NULL) {
            continue;
        }
#if defined(HASH_FUNC) && HASH_FUNC == MEIYAN
        hash = meiyan_hash(ht->items[i]->key);
#else
        hash = fnv1a_hash(ht->items[i]->key);
#endif
        index = hash % new_size;
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
#if defined(HASH_FUNC) && HASH_FUNC == MEIYAN
    size_t hash = meiyan_hash(key);
#else
    size_t hash = fnv1a_hash(key);
#endif
    size_t index = hash % ht->size;

    for (int i = 0; i < ht->size; i++) {
        if (ht->items[index] != NULL
            && ht->items[index]->key != NULL
            && ht->comparator(key, ht->items[index]->key, ht->items[index]) == 0) {
            return ht->items[index];
        }
        index = (index + 1) % ht->size;
    }
    return NULL;
}

void* ht_delete_entry(AM_ALLOCATOR_PARAM HashTable* ht, size_t index) {
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
#if defined(HASH_FUNC) && HASH_FUNC == MEIYAN
    size_t hash = meiyan_hash(key);
#else
    size_t hash = fnv1a_hash(key);
#endif
    size_t index = hash % ht->size;

    for (int i = 0; i < ht->size; i++) {
        if (ht->items[index] != NULL
            && ht->items[index]->key != NULL
            && ht->comparator(key, ht->items[index]->key, ht->items[index]) == 0) {
            return ht_delete_entry(AM_ALLOCATOR_ARG ht, index);
        }
        index = (index + 1) % ht->size;
    }
    return NULL;
}