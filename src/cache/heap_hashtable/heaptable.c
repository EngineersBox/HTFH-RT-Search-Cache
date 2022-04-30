#include "heaptable.h"

#include "../../math_utils.h"
#include "atomic_utils.h"

HeapTable* hpt_create(size_t ht_size) {
    HeapTable* hpt = malloc(sizeof(*hpt));
    if (hpt == NULL) {
        return NULL;
    } else if ((hpt->ht = ht_create(ht_size)) == NULL) {
        return NULL;
    } else if ((hpt->heap = calloc(ht_size, sizeof(hpt->heap[0]))) == NULL) {
        return NULL;
    }
    return hpt;
}

int hpt_contains(HeapTable* hpt, const char* key) {
    if (hpt == NULL || hpt->ht == NULL || key == NULL) {
        return -1;
    }
    return ht_get(hpt->ht, key) != NULL ? 0 : -1;
}

void* hpt_get(HeapTable* hpt, const char* key) {
    if (hpt == NULL || hpt->ht == NULL || key == NULL) {
        return NULL;
    }
    DQHTEntry* entry = ht_get(hpt->ht, key);
    return entry != NULL ? entry->ptr : NULL;
}

int hpt_insert(HeapTable* hpt, const char* key, void* value) {
    if (hpt == NULL || key == NULL) {
        return -1;
    }
    // TODO: Refactor this to avoid searching for keys more than once
    if (hpt_contains(hpt, key) == 0) {
        hpt_update(hpt, key, value);
    } else {
        hpt_push(hpt, key, value);
    }
    return 0;
}

int hpt_delete(HeapTable* hpt, const char* key) {
    if (hpt == NULL|| key == NULL) {
        return -1;
    }
    return hpt_remove(hpt, key);
}

int hpt_remove(HeapTable* hpt, const char* key) {
    if (hpt == NULL || hpt->ht == NULL || key == NULL) {
        return -1;
    }
    DQHTEntry* entry = ht_get(hpt->ht, key);
    if (entry == NULL) {
        return -1;
    }
    HeapEntry* last = hpt->heap[math_max(hpt->ht->count - 1, 0)];
    if (last == NULL || last->value) {
        return -1;
    }
    _Atomic(ptr_pair) pair = { NULL };
    atomic_init(&pair, { {entry, last} });
    atomic_swap(&pair);
}

void hpt_push(HeapTable* hpt, const char* key, void* value) {

}

void hpt_update(HeapTable* hpt, const char* key, void* value) {

}

void hpt_destroy(HeapTable* hpt) {
    if (hpt == NULL) {
        return;
    } else if (hpt->ht != NULL) {
        ht_destroy(hpt->ht);
    }
    free(hpt->heap);
    free(hpt);
}
