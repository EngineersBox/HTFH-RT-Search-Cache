#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_CACHE_HASHENTRY_H
#define HTFH_RT_SEARCH_CACHE_CACHE_HASHENTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

typedef struct HTEntry {
    char* key;
    size_t length;
    void* ptr;
} HTEntry;

HTEntry* htentry_create(const char* key, void* ptr);
void htentry_destroy(HTEntry* entry);

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_CACHE_HASHENTRY_H
