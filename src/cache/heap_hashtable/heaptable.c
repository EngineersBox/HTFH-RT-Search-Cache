#include "heaptable.h"

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

void hpt_destroy(HeapTable* hpt) {
    if (hpt == NULL) {
        return;
    } else if (hpt->ht != NULL) {
        ht_destroy(hpt->ht);
    }
    free(hpt->heap);
    free(hpt);
}
