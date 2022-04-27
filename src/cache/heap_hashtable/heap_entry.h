#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_HEAP_ENTRY_H
#define HTFH_RT_SEARCH_CACHE_HEAP_ENTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>

typedef struct HeapEntry {
    char* key;
    size_t length;
    void* value;
    int index;
} HeapEntry;

HeapEntry* he_create(const char* key, void* value);
void he_destroy(HeapEntry* entry);

// -1 = failure, 0 = less than, 1 = not less than
int he_less_than(HeapEntry* entry1, HeapEntry* entry2);

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_HEAP_ENTRY_H
