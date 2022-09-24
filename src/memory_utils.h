#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_MEMORY_UTILS_H
#define HTFH_RT_SEARCH_CACHE_MEMORY_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>

static void dump(void *myStruct, long size) {
    unsigned int i;
    const unsigned char* const px = (unsigned char*) myStruct;
    for (i = 0; i < size; ++i) {
        if( i % (sizeof(int) * 8) == 0){
            printf("\n%08X ", i);
        } else if( i % 4 == 0){
            printf(" ");
        }
        printf("%02X", px[i]);
    }
    printf("\n\n");
}

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_MEMORY_UTILS_H
