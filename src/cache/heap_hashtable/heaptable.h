#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_HEAPTABLE_H
#define HTFH_RT_SEARCH_CACHE_HEAPTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>

#include "../hashtable/cache_hashtable.h"
#include "heap_entry.h"

typedef struct HeapTable {
    HashTable* ht;
    HeapEntry** heap;
} HeapTable;

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_HEAPTABLE_H
