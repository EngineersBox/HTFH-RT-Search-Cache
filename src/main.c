#include <stdlib.h>
#include <pthread.h>

#include "logging/logging.h"
//LOGS_DIR("/mnt/e/HTFH-RT-Search-Cache/logs");

#include "cache/cache.h"
#include "cache/cache_backing.h"
#include "allocator/error/allocator_errno.h"

#define HEAP_SIZE (16 * 10000)

#define AND_OP '^'
#define OR_OP_STR "|"
#define OR_OP OR_OP_STR[0]

#define THREAD_COUNT 1

int key_compare(const char* key1, const char* key2) {
    if (key1 == NULL && key2 == NULL) {
        return 0;
    } else if (key1 == NULL) {
        return -1;
    } else if (key2 == NULL) {
        return 1;
    } else if (strchr(key2, AND_OP) != NULL || strchr(key2, OR_OP) == NULL) {
        return strcmp(key1, key2);
    }
    char* token = strtok(key2, OR_OP_STR);
    while (token != NULL) {
        if (strcmp(key1, token) == 0) {
            return 0;
        }
        token = strtok(key2, OR_OP_STR);
    }
    return 1;
}

static void locked_dqht_print_table(Cache* cache, char* prefix, DequeueHashTable* dqht) {
//    if (htfh_rwlock_rdlock_handled(&cache->rwlock) != 0) {
//        return;
//    }
//    dqht_print_table(prefix, dqht);
//    htfh_rwlock_unlock_handled(&cache->rwlock);
}

static pthread_barrier_t barrier;
char* to_store[10] = {
    "test1",
    "other",
    "more",
    "something",
    "yeahd",
    "dgsdfs",
    "ehjigdss",
    "oijfguod",
    "ngujhdw",
    "doij42od"
};
int values[10] = {
    56135,
    7345,
    5246,
    16473,
    9537,
    24675,
    55363,
    426723,
    957357,
    64526
};

typedef struct Params {
    int index;
    Cache* cache;
} Params;

void* threadFn(void* arg) {
    Params* params = (Params*) arg;
    Cache* cache = (Cache*) params->cache;
    int index = params->index;
    DEBUG("======== BEFORE WAITING ========");
    pthread_barrier_wait(&barrier);
    DEBUG("======== AFTER WAITING ========");
    INFO("======== REQUEST 1 ========");
    locked_dqht_print_table(cache, "Non-Resident HIRS", cache->backing->non_resident_hirs);
    locked_dqht_print_table(cache, "LIRS", cache->backing->lirs);
    locked_dqht_print_table(cache, "Resident HIRS", cache->backing->resident_hirs);
    for (int i = 0; i < 10; i++) {
        for (int j = 1; j < i + 1; j++) {
            DLIRSEntry* match = NULL;
            DLIRSEntry* evicted = NULL;
            int requestResult;
            printf("==== THREAD: %d ====\n", index);
            if ((requestResult = cache_query(cache, to_store[i], (void**) &match, (void**) &evicted)) == -1) {
                ERROR("[%d:%d] Failure occurred while retrieving cache entry for %s", i, j, to_store[i]);
                dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
                cache_destroy(cache);
                pthread_kill(pthread_self(), 1);
                return 0;
            } else if (requestResult == 1) {
                INFO("[%d:%d] Cache contains entry already for %s", i, j, to_store[i]);
                LOCALISE_ALLOCATOR_ARG
                dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
                continue;
            }
            INFO("[%d:%d] Cache does not contain entry for %s, requesting population", i, j, to_store[i]);
            LOCALISE_ALLOCATOR_ARG
            dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
            evicted = NULL;
            if ((requestResult = cache_request(cache, to_store[i], &values[i], (void**) &evicted)) == -1) {
                ERROR("[%d:%d] Unable to request cache population for [%s: %d] [Evicted: %p]", i, j, to_store[i], values[i], evicted);
                LOCALISE_ALLOCATOR_ARG
                dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
                cache_destroy(cache);
                pthread_kill(pthread_self(), 1);
                return 0;
            }
            INFO("[%d:%d] Request result: %s with evicted: %p for [%s: %d]", i, j, requestResult == 1 ? "hit" : "miss", evicted, to_store[i], values[i]);
            locked_dqht_print_table(cache, "Non-Resident HIRS", cache->backing->non_resident_hirs);
            locked_dqht_print_table(cache, "LIRS", cache->backing->lirs);
            locked_dqht_print_table(cache, "Resident HIRS", cache->backing->resident_hirs);
            LOCALISE_ALLOCATOR_ARG
            dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
            TRACE("[%d:%d] Cache is full? %s", i, j, cache_is_full(cache) ? "true" : "false");
        }
    }
    INFO("======== CONTAINS ========");
    locked_dqht_print_table(cache, "Non-Resident HIRS", cache->backing->non_resident_hirs);
    locked_dqht_print_table(cache, "LIRS", cache->backing->lirs);
    locked_dqht_print_table(cache, "Resident HIRS", cache->backing->resident_hirs);
    for (int i = 0; i < 10; i++) {
        INFO("==== THREAD: %d ====", index);
        void* match = cache_get(cache, to_store[i]);
        INFO("Cache contains %s: %s [Value: %p]", to_store[i], match != NULL ? "true" : "false", match);
    }
    INFO("======== REQUEST 2 ========");
    for (int i = 0; i < 10; i++) {
        DLIRSEntry* match = NULL;
        DLIRSEntry* evicted = NULL;
        int requestResult;
        printf("==== THREAD: %d ====\n", index);
        if ((requestResult = cache_query(cache, to_store[i], (void**) &match, (void**) &evicted)) == -1) {
            ERROR("[%d] Failure occurred while retrieving cache entry for %s", i, to_store[i]);
            dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
            cache_destroy(cache);
            pthread_kill(pthread_self(), 1);
            return 0;
        } else if (requestResult == 1) {
            INFO("[%d] Cache contains entry already for %s", i, to_store[i]);
            LOCALISE_ALLOCATOR_ARG
            dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
            continue;
        }
        INFO("[%d] Cache does not contain entry for %s, requesting population", i, to_store[i]);
        LOCALISE_ALLOCATOR_ARG
        dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
        evicted = NULL;
        if ((requestResult = cache_request(cache, to_store[i], &values[i], (void**) &evicted)) == -1) {
            ERROR("[%d] Unable to request cache population for [%s: %d] [Evicted: %p]", i, to_store[i], values[i], evicted);
            LOCALISE_ALLOCATOR_ARG
            dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
            cache_destroy(cache);
            pthread_kill(pthread_self(), 1);
            return 0;
        }
        INFO("[%d] Request result: %s with evicted: %p for [%s: %d]", i, requestResult == 1 ? "hit" : "miss", evicted, to_store[i], values[i]);
        locked_dqht_print_table(cache, "Non-Resident HIRS", cache->backing->non_resident_hirs);
        locked_dqht_print_table(cache, "LIRS", cache->backing->lirs);
        locked_dqht_print_table(cache, "Resident HIRS", cache->backing->resident_hirs);
        LOCALISE_ALLOCATOR_ARG
        dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
        TRACE("[%d] Cache is full? %s", i, cache_is_full(cache) ? "true" : "false");
    }
    INFO("<><><><> END OF TEST <><><><>");
    pthread_exit(0);
}

int main(int argc, char* argv[]) {
    srand(time(0));
    Cache* cache = cache_create(
        HEAP_SIZE,
        8,
        4,
        DLIRS_CACHE_BACKING_HANDLERS,
        &(DLIRSOptions) {
            .hirs_ratio = 0.01f,
            .comparator = key_compare
        }
    );
    if (cache == NULL) {
        FATAL("Could not create cache with size 10");
    }

    if (pthread_barrier_init(&barrier, NULL, THREAD_COUNT) != 0) {
        FATAL("Could not create thread barrier");
    }

    pthread_t threadIds[THREAD_COUNT];
    DEBUG("Created thread attributes");
    Params params[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; i++) {
        params[i] = (Params){
            .index = i,
            .cache = cache
        };
        if (pthread_create(&threadIds[i], NULL, threadFn, (void*) &params[i]) != 0) {
            FATAL("Could not create thread\n");
        }
        DEBUG("Created thread %d", i);
    }
    INFO("======== BEFORE JOIN ========");
    int retVals[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threadIds[i], &retVals[i]);
        if (retVals[i] != 0) {
            return 1;
        }
    }
    INFO("======== AFTER JOIN ========");
    if (pthread_barrier_destroy(&barrier) != 0) {
        FATAL("Could not destroy thread barrier");
    }

    INFO("======== CLEANUP ========");
    locked_dqht_print_table(cache, "Non-Resident HIRS", cache->backing->non_resident_hirs);
    locked_dqht_print_table(cache, "LIRS", cache->backing->lirs);
    locked_dqht_print_table(cache, "Resident HIRS", cache->backing->resident_hirs);
    if (cache_destroy(cache) != 0) {
        FATAL("Could not destroy cache");
    }
    return 0;
}