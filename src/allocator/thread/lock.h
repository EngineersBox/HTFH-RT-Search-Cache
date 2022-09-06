#pragma once

#ifndef H_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_THREAD_LOCK
#define H_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_THREAD_LOCK

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <sys/errno.h>

// ==== MUTEX ====

typedef pthread_mutex_t __attribute__((__may_alias__)) htfh_lock_t;

#define htfh_lock_init(lock, type) ({ \
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

#define htfh_lock_lock(lock) pthread_mutex_lock(lock)
#define htfh_lock_unlock(lock) pthread_mutex_unlock(lock)

#define htfh_lock_lock_handled(lock) ({ \
    int _lock_result = 0; \
    if (htfh_lock_lock(lock) == EINVAL) { \
        set_alloc_errno_msg(MUTEX_LOCK_LOCK, strerror(EINVAL)); \
        _lock_result = -1; \
    } \
    _lock_result; \
})

#define htfh_lock_unlock_handled(lock) ({ \
    int _unlock_result = 0; \
    if ((_unlock_result = htfh_lock_unlock(lock)) != 0) { \
        set_alloc_errno_msg(MUTEX_LOCK_UNLOCK, strerror(_unlock_result)); \
        _unlock_result = -1; \
    } \
    _unlock_result; \
})

// ==== RWLOCK ====

typedef pthread_rwlock_t htfh_rwlock_t;

#define htfh_rwlock_init(lock, pshared) ({ \
    int result = 0; \
    pthread_rwlockattr_t attr; \
    if ((result = pthread_rwlockattr_init(&attr)) == 0) { \
        if ((result = pthread_rwlockattr_setpshared(&attr, pshared)) == 0) { \
            if ((result = pthread_rwlock_init(lock, &attr)) == 0) { \
                result = pthread_rwlockattr_destroy(&attr) == EINVAL ? EINVAL : 0; \
            } \
        } \
    } \
    result; \
})

#define htfh_rwlock_rdlock(lock) pthread_rwlock_rdlock(lock)
#define htfh_rwlock_wrlock(lock) pthread_rwlock_wrlock(lock)
#define htfh_rwlock_unlock(lock) pthread_rwlock_unlock(lock)
#define htfh_rwlock_destroy(lock) pthread_rwlock_destroy(lock)

#define htfh_rwlock_rdlock_handled(lock) ({ \
    int _lock_result = 0; \
    if (htfh_rwlock_rdlock(lock) != 0) { \
        set_alloc_errno_msg(RWLOCK_WRLOCK_LOCK, strerror(_lock_result)); \
        _lock_result = -1; \
    } \
    _lock_result; \
})

#define htfh_rwlock_wrlock_handled(lock) ({ \
    int _lock_result = 0; \
    if (htfh_rwlock_wrlock(lock) != 0) { \
        set_alloc_errno_msg(RWLOCK_RDLOCK_LOCK, strerror(_lock_result)); \
        _lock_result = -1; \
    } \
    _lock_result; \
})

#define htfh_rwlock_unlock_handled(lock) ({ \
    int _unlock_result = 0; \
    if ((_unlock_result = htfh_rwlock_unlock(lock)) != 0) { \
        set_alloc_errno_msg(RWLOCK_LOCK_UNLOCK, strerror(_unlock_result)); \
        _unlock_result = -1; \
    } \
    _unlock_result; \
})

#define htfh_rwlock_destroy_handled(lock) ({ \
    int _unlock_result = 0; \
    if ((_unlock_result = htfh_rwlock_destroy(lock)) != 0) { \
        set_alloc_errno_msg(RWLOCK_LOCK_UNLOCK, strerror(_unlock_result)); \
        _unlock_result = -1; \
    } \
    _unlock_result; \
})

#ifdef __cplusplus
};
#endif

#endif // H_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_THREAD_LOCK