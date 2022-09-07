#pragma once

#ifndef _H_C_FIXED_HEAP_ALLOCATOR_STATIC_ALLOCATOR_
#define _H_C_FIXED_HEAP_ALLOCATOR_STATIC_ALLOCATOR_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include "glibc.h"
#include "../error/allocator_errno.h"

#ifdef STATIC_CFH

#ifndef STATIC_CFH_HEAP_SIZE
#define STATIC_CFH_HEAP_SIZE 100000
#endif

static Allocator* CFH_ALLOCATOR = NULL;

__attribute__((__constructor__
#ifdef STATIC_CFH_CONSTRUCTOR_PRIORITY
(STATIC_CFH_CONSTRUCTOR_PRIORITY)
#endif
)) int __static_cfh_constructor() {
    CFH_ALLOCATOR = malloc(sizeof(*CFH_ALLOCATOR));
    if (cfh_new(CFH_ALLOCATOR) == -1) {
        alloc_perror("");
        return 1;
    }
    if (cfh_init(CFH_ALLOCATOR, FIRST_FIT, STATIC_CFH_HEAP_SIZE) == -1) {
        alloc_perror("Initialisation failed for heap :");
        return 1;
    }
    return 0;
}

__attribute__((__destructor__
#ifdef STATIC_CFH_DESTRUCTOR_PRIORITY
(STATIC_CFH_DESTRUCTOR_PRIORITY)
#endif
)) int __static_cfh_destructor() {
    if (cfh_destruct(CFH_ALLOCATOR) == -1) {
        alloc_perror("");
        return 1;
    }
    return 0;
}

#endif

#ifdef __cplusplus
};
#endif

#endif // _H_C_FIXED_HEAP_ALLOCATOR_STATIC_ALLOCATOR_