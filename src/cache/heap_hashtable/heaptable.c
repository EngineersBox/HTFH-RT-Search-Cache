//#include "heaptable.h"
//
//#include <string.h>
//
//#include "../../math_utils.h"
//#include "atomic_utils.h"
//
//HeapTable* hpt_create(size_t ht_size) {
//    HeapTable* hpt = malloc(sizeof(*hpt));
//    if (hpt == NULL) {
//        return NULL;
//    } else if ((hpt->ht = ht_create(ht_size)) == NULL) {
//        return NULL;
//    } else if ((hpt->heap = calloc(ht_size, sizeof(hpt->heap[0]))) == NULL) {
//        return NULL;
//    }
//    hpt->heapSize = ht_size;
//    hpt->heapCount = 0;
//    return hpt;
//}
//
//int hpt_contains(HeapTable* hpt, const char* key) {
//    if (hpt == NULL || hpt->ht == NULL || key == NULL) {
//        return -1;
//    }
//    return ht_get(hpt->ht, key) != NULL ? 0 : -1;
//}
//
//void* hpt_get(HeapTable* hpt, const char* key) {
//    if (hpt == NULL || hpt->ht == NULL || key == NULL) {
//        return NULL;
//    }
//    DQHTEntry* entry = ht_get(hpt->ht, key);
//    return entry != NULL ? entry->ptr : NULL;
//}
//
//int hpt_insert(HeapTable* hpt, const char* key, void* value) {
//    if (hpt == NULL || key == NULL) {
//        return -1;
//    }
//    // TODO: Refactor this to avoid searching for keys more than once
//    if (hpt_contains(hpt, key) == 0) {
//        hpt_update(hpt, key, value);
//    } else {
//        hpt_push(hpt, key, value);
//    }
//    return 0;
//}
//
//int hpt_delete(HeapTable* hpt, const char* key) {
//    if (hpt == NULL|| key == NULL) {
//        return -1;
//    }
//    return hpt_remove(hpt, key);
//}
//
//int hpt_remove(HeapTable* hpt, const char* key) {
//    if (hpt == NULL || hpt->ht == NULL || key == NULL) {
//        return -1;
//    }
//    DQHTEntry* entry = ht_get(hpt->ht, key);
//    if (entry == NULL) {
//        return -1;
//    }
//    HeapEntry* last = hpt->heap[math_max(hpt->ht->count - 1, 0)];
//    if (last == NULL || last->value) {
//        return -1;
//    }
//    _Atomic(ptr_pair) pair = { NULL };
//    atomic_init(&pair, {entry, last});
//    atomic_swap(&pair);
//    if (strcmp(entry->key, last->key) != 0) {
//        hpt_heapify_up(hpt, last->index);
//        hpt_heapify(hpt, last->index);
//    }
//    int result = ht_delete(hpt->ht, key) != NULL;
//    hpt->heapCount--;
//    return result;
//}
//
//int hpt_resize(HeapTable* hpt) {
//    if (hpt->heapCount < (hpt->heapSize / 2)) {
//        return 0;
//    }
//    return (hpt->heap = realloc(hpt->heap, hpt->heapSize *= 2)) != NULL
//}
//
//int hpt_push(HeapTable* hpt, const char* key, void* value) {
//    if (hpt == NULL || hpt->ht == NULL || key == NULL) {
//        return -1;
//    }
//    DQHTEntry* entry = ht_get(hpt->ht, key);
//    if (entry != NULL) {
//        return -1;
//    }
//    DQHTEntry* newEntry = ht_insert(hpt->ht, key, value);
//    HeapEntry* heapEntry = he_create(key, value);
//    newEntry->ptr = heapEntry;
//    if (!hpt_resize(hpt)) {
//        return -1;
//    }
//    heapEntry->index = hpt->heapCount;
//    hpt->heap[heapEntry->index] = heapEntry;
//    hpt_heapify_up(hpt, heapEntry->index);
//    hpt->heapCount++;
//    return 0;
//}
//
//int hpt_update(HeapTable* hpt, const char* key, void* value) {
//    if (hpt == NULL || hpt->ht == NULL || key == NULL) {
//        return -1;
//    }
//    DQHTEntry* entry = ht_get(hpt->ht, key);
//    if (entry == NULL) {
//        return -1;
//    }
//    HeapEntry* heapEntry = entry->ptr;
//    heapEntry->value = value;
//    hpt_heapify_up(hpt, heapEntry->index);
//    hpt_heapify(hpt, heapEntry->index);
//    return 0;
//}
//
//void* hpt_min(HeapTable* hpt) {
//    if (hpt == NULL || hpt->ht == NULL || hpt->heapSize == 0  || hpt->heapCount == 0) {
//        return NULL;
//    }
//    HeapEntry* entry = hpt->heap[0];
//    if (entry == NULL) {
//        return NULL;
//    }
//    return entry->value;
//}
//
//void* hpt_pop_min(HeapTable* hpt) {
//    if (hpt == NULL || hpt->ht == NULL || hpt->heapSize == 0 || hpt->heapCount == 0) {
//        return NULL;
//    }
//    HeapEntry* entry = hpt->heap[0];
//    if (entry == NULL) {
//        return NULL;
//    }
//    void* value = entry->value;
//    if (hpt_remove(hpt, entry->key) == -1) {
//        return NULL;
//    }
//    he_destroy(entry);
//    return value;
//}
//
//HeapEntry* hpt_parent(HeapTable* hpt, size_t index) {
//    if (hpt == NULL || hpt->ht == NULL) {
//        return NULL;
//    }
//    const int parentIndex = (index - 1) / 2;
//    return hpt->heap[parentIndex];
//}
//
//HeapEntry* hpt_child_left(HeapTable* hpt, size_t index) {
//    if (hpt == NULL || hpt->ht == NULL) {
//        return NULL;
//    }
//    const int leftIndex = (2 * index) + 1;
//    if (leftIndex < hpt->heapCount) {
//        return hpt->heap[leftIndex];
//    }
//    return NULL;
//}
//
//HeapEntry* hpt_child_right(HeapTable* hpt, size_t index) {
//    if (hpt == NULL || hpt->ht == NULL) {
//        return NULL;
//    }
//    const int rightIndex = (2 * index) + 2;
//    if (rightIndex < hpt->heapCount) {
//        return hpt->heap[leftIndex];
//    }
//    return NULL;
//}
//
//void hpt_heapify_up(HeapTable* hpt, size_t index) {
//    if (hpt == NULL || hpt->ht == NULL) {
//        return;
//    }
//    HeapEntry* parent = hpt_parent(hpt, index);
//    HeapEntry* entry = hpt->heap[index];
//    _Atomic(ptr_pair) pair = { NULL };
//    while (parent != NULL && entry->index > 0 && entry->value < parent->value) {
//        pair = {NULL};
//        atomic_init(&pair, {entry, parent});
//        atomic_swap(&pair);
//        parent = hpt_parent(hpt, entry->index);
//    }
//}
//
//void hpt_heapify(HeapTable* hpt, size_t index) {
//    // TODO
//}
//
//void hpt_destroy(HeapTable* hpt) {
//    if (hpt == NULL) {
//        return;
//    } else if (hpt->ht != NULL) {
//        ht_destroy(hpt->ht);
//    }
//    free(hpt->heap);
//    free(hpt);
//}
