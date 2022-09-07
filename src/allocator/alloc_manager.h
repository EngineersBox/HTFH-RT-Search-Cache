#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_ALLOC_MANAGER_H
#define HTFH_RT_SEARCH_CACHE_ALLOC_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ALLOCATOR_TYPE
#define ALLOCATOR_TYPE 0
#endif

#if ALLOCATOR_TYPE == 0
#include <stdlib.h>
#define AM_ALLOCATOR_PARAM
#define AM_ALLOCATOR_ARG
#define am_malloc(size) malloc(size)
#define am_calloc(count, size) calloc(count, size)
#define am_memalign(align, size) aligned_alloc(align, size)
#define am_realloc(ptr, size) realloc(ptr, size)
#define am_free(ptr) free(ptr)
#elif ALLOCATOR_TYPE == 1
#include "htfh/htfh.h"
#define AM_ALLOCATOR_PARAM Allocator* allocator,
#define AM_ALLOCATOR_ARG allocator,
#define am_malloc(size) htfh_malloc(AM_ALLOCATOR_ARG size)
#define am_calloc(count, size) htfh_calloc(AM_ALLOCATOR_ARG count, size)
#define am_memalign(align, size) htfh_memalign(AM_ALLOCATOR_ARG align, size)
#define am_realloc(ptr, size) htfh_realloc(AM_ALLOCATOR_ARG ptr, size)
#define am_free(ptr) htfh_free(AM_ALLOCATOR_ARG ptr)
#elif ALLOCATOR_TYPE == 2
#include "glibc/glibc.h"
#define AM_ALLOCATOR_PARAM GlibcAllocator* allocator,
#define AM_ALLOCATOR_ARG allocator,
#define am_malloc(size) gca_malloc(AM_ALLOCATOR_ARG size)
#define am_calloc(count, size) gca_calloc(AM_ALLOCATOR_ARG count, size)
#define am_memalign(align, size) NULL
#define am_realloc(ptr, size) NULL
#define am_free(ptr) gca_free(AM_ALLOCATOR_ARG ptr)
#endif

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_ALLOC_MANAGER_H
