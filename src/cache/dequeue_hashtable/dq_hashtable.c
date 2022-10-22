#include "dq_hashtable.h"

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "../../logging/logging.h"
#include "../hashtable/hashing.h"
#include "../cache_key.h"
#include "../dlirs/dlirs_entry.h"

int default_comparator(const char* key1, const char* key2, void* _ignored) {
    return key_cmp(key1, key2);
}

DequeueHashTable* dqht_create(AM_ALLOCATOR_PARAM size_t size, KeyComparator comparator) {
    HashTable* ht = ht_create(AM_ALLOCATOR_ARG size, comparator == NULL ? default_comparator : comparator);
    if (ht == NULL) {
        return NULL;
    }
    DequeueHashTable* dqht = (DequeueHashTable*) am_malloc(sizeof(*dqht));
    if (dqht == NULL) {
        return NULL;
    }
    dqht->keyComparator = comparator == NULL ? default_comparator : comparator;
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
    DQHTEntry* entry = NULL;
    int result = ht_insert(AM_ALLOCATOR_ARG dqht->ht, key, value, &entry);
    if (result == -1) {
        return -1;
    } else if (result == 1) {
        dqht_unlink(dqht, entry);
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
            && dqht->keyComparator(key, dqht->ht->items[index]->key, dqht->ht->items[index]) == 0) {
            dqht_unlink(dqht, dqht->ht->items[index]);
            return ht_delete_entry(AM_ALLOCATOR_ARG dqht->ht, index);
        }
        index = (index + 1) % dqht->ht->size;
    }
    return NULL;
}

void* dqht_get_front(DequeueHashTable* dqht) {
    return dqht == NULL || dqht->head == NULL || DQHT_STRICT_CHECK(dqht) ? NULL : dqht->head->ptr;
}

int dqht_push_front(AM_ALLOCATOR_PARAM DequeueHashTable* dqht, const char* key, void* value) {
    if (dqht == NULL || key == NULL || DQHT_STRICT_CHECK(dqht)) {
        return -1;
    }
    DQHTEntry* entry = NULL;
    int result = ht_insert(AM_ALLOCATOR_ARG dqht->ht, key, value, &entry);
    if (result == -1) {
        return -1;
    } else if (result == 1) {
        dqht_unlink(dqht, entry);
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
    TRACE("[BEFORE SET] Current head: %p %zu", dqht->head, dqht->head->index);
    DQHTEntry* front = dqht->head;
    TRACE("[AFTER SET] Current head: %p %zu", dqht->head, dqht->head->index);
    TRACE("[BEFORE UNLINK] Front: %p %zu", front, front->index);
    dqht_unlink(dqht, front);
    TRACE("[AFTER UNLINK] Front: %p %zu", front, front->index);
    return ht_delete_entry(AM_ALLOCATOR_ARG dqht->ht, front->index);
}

void* dqht_get_last(DequeueHashTable* dqht) {
    return dqht == NULL || dqht->tail == NULL || DQHT_STRICT_CHECK(dqht) ? NULL : dqht->tail->ptr;
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
        char* keyValue = key_sprint(dqht->head->key);
        char* keyValue1 = dqht->head->next == NULL ? NULL : key_sprint(dqht->head->next->key);
        char* keyValue2 = dqht->head->prev == NULL ? NULL : key_sprint(dqht->head->prev->key);
        printf(
            "[Head: [%zu] %s: %p (N: %s %p, P: %s %p)%s]\n",
            dqht->head->index,
            keyValue,
            dqht->head->ptr,
            keyValue1 != NULL ? keyValue1 : "",
            dqht->head->next == NULL ? NULL : dqht->head->next->ptr,
            keyValue2 != NULL ? keyValue2 : "",
            dqht->head->prev == NULL ? NULL : dqht->head->prev->ptr,
            dqht->head->next != NULL ? "," : " "
        );
        free(keyValue);
        if (keyValue1 != NULL) {
            free(keyValue1);
        }
        if (keyValue2 != NULL) {
            free(keyValue2);
        }
    } else {
        printf("[Head: (nil)]\n");
    }
    if (dqht->tail != NULL) {
        char* keyValue = key_sprint(dqht->tail->key);
        char* keyValue1 = dqht->tail->next == NULL ? NULL : key_sprint(dqht->tail->next->key);
        char* keyValue2 = dqht->tail->prev == NULL ? NULL : key_sprint(dqht->tail->prev->key);
        printf(
            "[Tail: [%zu] %s: %p (N: %s %p, P: %s %p)%s]\n",
            dqht->tail->index,
            keyValue,
            dqht->tail->ptr,
            keyValue1 != NULL ? keyValue1 : "",
            dqht->tail->next == NULL ? NULL : dqht->tail->next->ptr,
            keyValue2 != NULL ? keyValue2 : "",
            dqht->tail->prev == NULL ? NULL : dqht->tail->prev->ptr,
            dqht->tail->next != NULL ? "," : " "
        );
        free(keyValue);
        if (keyValue1 != NULL) {
            free(keyValue1);
        }
        if (keyValue2 != NULL) {
            free(keyValue2);
        }
    } else {
        printf("[Tail: (nil)]\n");
    }
    printf("{");
//    DQHTEntry* entry = dqht->head;
//    while (entry != NULL) {
//        char* keyValue = key_sprint(entry->key);
//        char* keyValue1 = entry->next == NULL ? NULL : key_sprint(entry->next->key);
//        char* keyValue2 = entry->prev == NULL ? NULL : key_sprint(entry->prev->key);
//        printf(
//            " [%zu] %s: %p (N: %s %p, P: %s %p)%s",
//            entry->index,
//            keyValue,
//            entry->ptr,
//            keyValue1 != NULL ? keyValue1 : "",
//            entry->next == NULL ? NULL : entry->next->ptr,
//            keyValue2 != NULL ? keyValue2 : "",
//            entry->prev == NULL ? NULL : entry->prev->ptr,
//            entry->next != NULL ? "," : " "
//        );
//        free(keyValue);
//        if (keyValue1 != NULL) {
//            free(keyValue1);
//        }
//        if (keyValue2 != NULL) {
//            free(keyValue2);
//        }
//        entry = entry->next;
//    }
    printf(" }\n");
}