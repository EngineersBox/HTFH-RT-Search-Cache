/*
** Two Level Segregated Fit memory allocator, version 3.1.
** Written by Matthew Conte
**	http://tlsf.baisoku.org
**
** Based on the original documentation by Miguel Masmano:
**	http://www.gii.upv.es/tlsf/main/docs
**
** This implementation was written to the specification
** of the document, therefore no GPL restrictions apply.
**
** Copyright (c) 2006-2016, Matthew Conte
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the copyright holder nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL MATTHEW CONTE BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** =============================================================================
** Modified and restructured by Jack Kilrain (EngineersBox) for use in HTFH
*/

#pragma once

#ifndef C_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR
#define C_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include "../thread/lock.h"
#include "controller.h"

typedef struct Allocator {
    htfh_lock_t mutex;
    Controller* controller;
    size_t heap_size;
    void* heap;
} Allocator;

typedef struct integrity_t {
    int prev_status;
    int status;
} integrity_t;

/* Create/destroy a memory pool. */
Allocator* htfh_create(size_t bytes);
int htfh_destroy(Allocator* alloc);

/* Add/remove memory pools. */
void* htfh_add_pool(Allocator* alloc, void* mem, size_t bytes);

#if defined(__GNUC__) \
    && __GNUC__ >= 10    \
    && (__GNUC__ > 10 || (__GNUC__ >= 0 && __GNUC_MINOR__ >= 0)) \
    && defined(__GNUC_PATCHLEVEL__)
#define gnu_version_10
#endif

/* malloc/memalign/realloc/free replacements. */
__attribute__((hot)) int htfh_free(Allocator* alloc, void* ptr);
__attribute__((hot, malloc
#ifdef gnu_version_10
, malloc (htfh_free, 2)
#endif
)) __attribute__((alloc_size(2))) void* htfh_malloc(Allocator* alloc, size_t bytes);
__attribute__((hot, malloc
#ifdef gnu_version_10
, malloc (htfh_free, 2)
#endif
)) __attribute__((alloc_size(2,3))) void* htfh_calloc(Allocator* alloc, size_t count, size_t bytes);
__attribute__((hot, malloc
#ifdef gnu_version_10
, malloc (htfh_free, 2)
#endif
)) __attribute__((alloc_size(2,3))) void* htfh_memalign(Allocator* alloc, size_t align, size_t bytes);
__attribute__((hot, malloc
#ifdef gnu_version_10
, malloc (htfh_free, 2)
#endif
)) __attribute__((alloc_size(3))) void* htfh_realloc(Allocator* alloc, void* ptr, size_t size);

#ifdef gnu_version_10
#undef gnu_version_10
#endif

/* Returns internal block size, not original request size */
size_t htfh_block_size(void* ptr);

/* Overheads/limits of internal structures. */
size_t htfh_size(void);
size_t htfh_align_size(void);
size_t htfh_block_size_min(void);
size_t htfh_block_size_max(void);
size_t htfh_pool_overhead(void);
size_t htfh_alloc_overhead(void);

#ifdef __cplusplus
};
#endif

#endif // C_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR