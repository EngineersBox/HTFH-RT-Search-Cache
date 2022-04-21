#include "dq_hashtable.h"

#include <stdlib.h>
#include "hashing.h"

DequeueHashTable* dqht_create(size_t size) {
    DequeueHashTable* dqht = malloc(sizeof(*dqht));
    dqht->size = size;
    dqht->count = 0;
    dqht->items = calloc(dqht->size, sizeof(DQHTEntry*));
    return dqht;
}

void dqht_destroy(DequeueHashTable* dqht) {
    if (dqht == NULL) {
        return;
    }
    for (int i = 0; i < dqht->size; i++) {
        DQHTEntry* entry = dqht->items[i];
        if (entry != NULL) {
            dqhtentry_destroy(entry);
        }
    }
    free(dqht->items);
    free(dqht);
}

DQHTEntry* __dqht_insert(DequeueHashTable* dqht, const char* key, void* value) {
    if (dqht == NULL) {
        return NULL;
    } else if (value == NULL) {
        return NULL;
    } else if (value == NULL) {
        return NULL;
    } else if (dqht->count >= (dqht->size / 2) && dqht_resize(dqht) != 0) {
        return NULL;
    }
    uint64_t hash = fnv1a_hash(key);
    size_t index = (size_t)(hash % ((uint64_t)(dqht->size - 1)));

    while(dqht->items[index] != NULL) {
        if (strcmp(key, dqht->items[index]->key) == 0) {
            void* current_value = dqht->items[index]->ptr;
            dqht->items[index]->ptr = value;
            return current_value;
        }
        index = (index + 1) % dqht->size;
    }
    dqht->items[index] = dqhtentry_create(key, value);
    if (dqht->items[index] == NULL) {
        return NULL;
    }
    dqht->count++;
    return value;
}

inline int dqht_resize_insert(DQHTEntry** items, size_t size, DQHTEntry* entry, size_t index) {
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

//void dqht_print_table(DequeueHashTable* dqht) {
//    for (int i = 0; i < dqht->size; i++) {
//        printf("Entry %d: %p\n", i, dqht->items[i]);
//    }
//}

int dqht_resize(DequeueHashTable* dqht) {
    size_t new_size = dqht->size * 2;
    if (new_size < dqht->size) {
        return -1;
    }
    DQHTEntry** new_items = calloc(new_size, sizeof(DQHTEntry*));
    if (new_items == NULL) {
        return -1;
    }
    uint64_t hash;
    size_t index;
    for (int i = 0; i < dqht->size; i++) {
        if (dqht->items[i] == NULL) {
            continue;
        }
        hash = fnv1a_hash(dqht->items[i]->key);
        index = (size_t)(hash % ((uint64_t)(new_size - 1)));
        if (dqht_resize_insert(new_items, new_size, dqht->items[i], index) != 0) {
            return -1;
        }
    }
    free(dqht->items);
    dqht->items = new_items;
    dqht->size = new_size;
    return 0;
}

void* dqht_get(DequeueHashTable* dqht, const char* key) {
    if (dqht == NULL) {
        return NULL;
    } else if (key == NULL) {
        return NULL;
    }
    uint64_t hash = fnv1a_hash(key);
    size_t index = (size_t)(hash & (uint64_t)(dqht->size - 1));

    for (int i = 0; i < dqht->size; i++) {
        if (dqht->items[index] != NULL && strcmp(key, dqht->items[index]->key) == 0) {
            return dqht->items[index]->ptr;
        }
        index = (index + 1) % dqht->size;
    }
    return NULL;
}

void* __dqht_delete(DequeueHashTable* dqht, const char* key) {
    if (dqht == NULL) {
        return NULL;
    } else if (key == NULL) {
        return NULL;
    }
    uint64_t hash = fnv1a_hash(key);
    size_t index = (size_t)(hash & (uint64_t)(dqht->size - 1));

    for (int i = 0; i < dqht->size; i++) {
        if (dqht->items[index] != NULL && strcmp(key, dqht->items[index]->key) == 0) {
            void* current_value = dqht->items[index]->ptr;
            dqhtentry_destroy(dqht->items[index]);
            return current_value;
        }
        index = (index + 1) % dqht->size;
    }
    return NULL;
}

int __dqht_remove(DequeueHashTable* dqht, const char* key) {
    if (dqht == NULL) {
        return -1;
    }
    DQHTEntry* entry = dqht_get(dqht, key);
    if (entry == NULL) {
        return -1;
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
    return __dqht_delete(dqht, key) == NULL ? 0 : -1;
}

int __dqht_push(DequeueHashTable* dqht, const char* key, void* value) {
    if (dqht == NULL) {
        return -1;
    }
    DQHTEntry* entry = dqht_get(dqht, key);
    if (entry != NULL) {
        return -1;
    }
    entry = __dqht_insert(dqht, key, value);
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

int __dqht_update(DequeueHashTable* dqht, const char* key, void* value) {
    if (dqht == NULL) {
        return -1;
    } else if (__dqht_remove(dqht, key) != 0) {
        return -1;
    } else if (__dqht_push(dqht, key, value) != 0) {
        return -1;
    }
    return 0;
}

int dqht_set(DequeueHashTable* dqht, const char* key, void* value) {
    if (dqht == NULL) {
        return -1;
    } else if (dqht_get(dqht, key) != NULL) {
        return __dqht_update(dqht, key, value);
    } else {
        return __dqht_push(dqht, key, value);
    }
}

int dqht_remove(DequeueHashTable* dqht, const char* key) {
    return __dqht_remove(dqht, key);
}

void* dqht_first(DequeueHashTable* dqht) {
    return dqht != NULL ? dqht->head : NULL;
}
int dqht_push_first(DequeueHashTable* dqht, const char* key, void* value) {
    if (dqht == NULL) {
        return -1;
    }
    DQHTEntry* entry = dqht_get(dqht, key);
    if (entry != NULL) {
        return -1;
    }
    entry = __dqht_insert(dqht, key, value);
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

void* dqht_pop_first(DequeueHashTable* dqht) {
    if (dqht == NULL) {
        return NULL;
    }
    DQHTEntry* first = dqht->head;
    if (first == NULL) {
        return NULL;
    }
    return __dqht_remove(dqht, first->key) == 0 ? first : NULL;
}

void* dqht_last(DequeueHashTable* dqht) {
    return dqht != NULL ? dqht->tail : NULL;
}

void* dqht_pop_last(DequeueHashTable* dqht) {
    if (dqht == NULL) {
        return NULL;
    }
    DQHTEntry* last = dqht->tail;
    if (last == NULL) {
        return NULL;
    }
    return __dqht_remove(dqht, last->key) == 0 ? last : NULL;
}