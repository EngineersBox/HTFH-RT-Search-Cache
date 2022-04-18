#pragma once

#ifndef _H_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_THREAD_LOCK_
#define _H_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_THREAD_LOCK_

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <sys/errno.h>

typedef pthread_mutex_t __htfh_lock_t;

#define __htfh_lock_init(lock, type) ({ \
    int result = 0; \
    pthread_mutexattr_t attr; \
    if ((result = pthread_mutexattr_init(&attr)) == 0) { \
        if ((result = pthread_mutexattr_settype(&attr, type)) == 0) { \
            if ((result = pthread_mutex_init(lock, &attr)) == 0) { \
                result = pthread_mutexattr_destroy(&attr) == EINVAL ? EINVAL : 0; \
            } \
        } \
    } \
    result; \
})

#define __htfh_lock_lock(lock) pthread_mutex_lock(lock)
#define __htfh_lock_unlock(lock) pthread_mutex_unlock(lock)

#define __htfh_lock_lock_handled(lock) ({ \
    int _lock_result = 0; \
    if (__htfh_lock_lock(lock) == EINVAL) { \
        set_alloc_errno_msg(MUTEX_LOCK_LOCK, strerror(EINVAL)); \
        _lock_result = -1; \
    } \
    _lock_result; \
})

#define __htfh_lock_unlock_handled(lock) ({ \
    int _unlock_result = 0; \
    if ((_unlock_result = __htfh_lock_unlock(lock)) != 0) { \
        set_alloc_errno_msg(MUTEX_LOCK_UNLOCK, strerror(_unlock_result)); \
        _unlock_result = -1; \
    } \
    _unlock_result; \
})

#ifdef __cplusplus
};
#endif

#endif // _H_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_THREAD_LOCK_