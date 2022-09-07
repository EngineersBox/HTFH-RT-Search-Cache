#include "glibc.h"
#include <stddef.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "../error/allocator_errno.h"

int gca_create(size_t heapSize) {
    GlibcAllocator* alloc = malloc(sizeof(*alloc));
    if (alloc == NULL) {
        set_alloc_errno(NULL_ALLOCATOR_INSTANCE);
        return -1;
    }
    init_check(int, lock_result, htfh_lock_init(&alloc->mutex, PTHREAD_MUTEX_RECURSIVE), != 0) {
        set_alloc_errno_msg(MUTEX_LOCK_INIT, strerror(lock_result));
        return NULL;
    } else if (htfh_lock_lock_handled(&alloc->mutex) != 0) {
        return NULL;
    }
    alloc->freep = NULL;
    alloc->heap = NULL;
    alloc->method = FIRST_FIT;
    alloc->heap_size = heapSize;
    alloc->current_brk = alloc->heap = mmap(
        NULL,
        heap_size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );
    if (alloc->heap == NULL) {
        set_alloc_errno(HEAP_MMAP_FAILED);
        htfh_lock_unlock_handled(&alloc->mutex);
        return -1;
    }
    return htfh_lock_unlock_handled(&alloc->mutex);
}

int gca_destroy(GlibcAllocator* alloc) {
    if (alloc == NULL) {
        set_alloc_errno(BAD_DEALLOC);
        return -1;
    } else if (htfh_lock_lock_handled(&alloc->mutex) == -1) {
        return -1;
    } else if (alloc->heap != NULL && munmap(alloc->heap, alloc->heap_size) != 0) {
        set_alloc_errno(HEAP_UNMAP_FAILED);
        htfh_lock_unlock_handled(&alloc->mutex);
        return -1;
    } else if (htfh_lock_unlock_handled(&alloc->mutex) == -1) {
        return -1;
    }
    free(alloc);
    return 0;
}

int gca_brk(GlibcAllocator* alloc, void* addr) {
    if (addr > (alloc->heap + (uintptr_t) alloc->heap_size)) {
        return -1;
    }
    alloc->current_brk = addr;
    return 0;
}

void* gca_sbrk(GlibcAllocator* alloc, intptr_t increment) {
    if (alloc->current_brk == NULL && gca_brk(alloc, 0) < 0) {
        return (void*) -1;
    } else if (increment == 0) {
        return alloc->current_brk;
    }
    void* oldbrk = alloc->current_brk;
    if (increment > 0
        ? ((uintptr_t) oldbrk + (uintptr_t) increment < (uintptr_t) oldbrk)
        : ((uintptr_t) oldbrk < (uintptr_t) -increment)){
        return (void*) -1;
    } else if (gca_brk(alloc, oldbrk + increment) < 0) {
        return (void*) -1;
    }
    return oldbrk;
}

int cfh_free(GlibcAllocator* alloc, void* ap) {
    if (htfh_lock_lock_handled(&alloc->mutex) == -1) {
        return -1;
    }

    Header*bp = (Header*) ap - 1;
    Header* p;
    for (p = alloc->freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr) {
        if (p >= p->s.ptr && (bp > p || bp < p->s.ptr)) {
            break;
        }
    }
    if (bp + bp->s.size == p->s.ptr) {
        bp->s.size += p->s.ptr->s.size;
        bp->s.ptr = p->s.ptr->s.ptr;
    } else {
        bp->s.ptr = p->s.ptr;
    }
    if (p + p->s.size == bp) {
        p->s.size += bp->s.size;
        p->s.ptr = bp->s.ptr;
    } else {
        p->s.ptr = bp;
    }
    alloc->freep = p;
    return htfh_lock_unlock_handled(&alloc->mutex);
}

Header* more_core(GlibcAllocator* alloc, unsigned int nu) {
    if (nu < NALLOC) {
        nu = NALLOC;
    }
    char* cp = gca_sbrk(alloc, nu * sizeof(Header));
    if (cp == (char*) - 1) {
        return NULL;
    }
    Header* up = (Header*) cp;
    up->s.size = nu;
    if (gca_free(alloc, (void*)(up + 1)) != 0) {
        return NULL;
    }
    return alloc->freep;
}

void* gca_malloc(GlibcAllocator* alloc, unsigned nbytes) {
    if (htfh_lock_lock_handled(&alloc->mutex) == -1) {
        return NULL;
    }
    unsigned nunits = (nbytes + sizeof(Header) + 1) / sizeof(Header) + 1;
    Header* prevp;

    if ((prevp = alloc->freep) == NULL) {
        alloc->base.s.ptr = alloc->freep = prevp = &alloc->base;
        alloc->base.s.size = 0;
    }

    for (Header* p = prevp->s.ptr;; prevp = p, p = p->s.ptr) {
        if (p->s.size >= nunits) {
            if (p->s.size == nunits) {
                prevp->s.ptr = p->s.ptr;
            } else {
                p->s.size -= nunits;
                p += p->s.size;
                p->s.size = nunits;
            }
            alloc->freep = prevp;
            return htfh_lock_unlock_handled(&alloc->mutex) == 0 ? (void*) (p+1) : NULL;
        }
        if (p == alloc->freep && ((p = more_core(alloc, nunits)) == NULL)) {
            set_alloc_errno(MALLOC_FAILED);
            htfh_lock_unlock_handled(&alloc->mutex);
            return NULL;
        }
    }
}

void* gca_calloc(GlibcAllocator* alloc, unsigned count, unsigned nbytes) {
    if (htfh_lock_lock_handled(&alloc->mutex) == -1) {
        return NULL;
    }
    void* ptr = cfh_malloc(alloc, count * nbytes);
    return htfh_lock_unlock_handled(&alloc->mutex) != 0 || ptr == NULL ? NULL : memset(ptr, 0, count * nbytes);
}