#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_ALLOC_MANAGER_H
#define HTFH_RT_SEARCH_CACHE_ALLOC_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HTFH_ALLOCATOR
#include "htfh/htfh.h"
#define AM_ALLOCATOR_PARAM Allocator* allocator,
#define AM_ALLOCATOR_ARG allocator,
#define am_malloc(size) htfh_malloc(AM_ALLOCATOR_ARG size)
#define am_calloc(count, size) htfh_calloc(AM_ALLOCATOR_ARG count, size)
#define am_memalign(align, size) htfh_memalign(AM_ALLOCATOR_ARG align, size)
#define am_realloc(ptr, size) htfh_realloc(AM_ALLOCATOR_ARG ptr, size)
#define am_free(ptr) htfh_free(AM_ALLOCATOR_ARG ptr)
#else
#include <stdlib.h>
#define AM_ALLOCATOR_PARAM
#define AM_ALLOCATOR_ARG
#define am_malloc(size) malloc(size)
#define am_calloc(count, size) calloc(count, size)
#define am_memalign(align, size) aligned_alloc(align, size)
#define am_realloc(ptr, size) realloc(ptr, size)
#define am_free(ptr) free(ptr)
#endif

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_ALLOC_MANAGER_H
