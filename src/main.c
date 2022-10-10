#include <stdlib.h>
#include <pthread.h>

#include "logging/logging.h"
#include "result.h"
#include "cache/cache_key.h"
//LOGS_DIR("/mnt/e/HTFH-RT-Search-Cache/logs");

#include "cache/cache.h"
#include "cache/cache_backing.h"
#include "allocator/error/allocator_errno.h"

#define HEAP_SIZE (16 * 10000)

#define AND_OP '^'
#define OR_OP '|'
#define SINGLE_OP '0'
#define IS_AND(op) (op == AND_OP)
#define IS_OR(op) (op == OR_OP)
#define IS_SINGLE(op) (op == SINGLE_OP)

int partialCacheMatches = 0;

#define THREAD_COUNT 10

int key_compare(const char* key1, const char* key2, void* key2Value) {
    if (key1 == NULL && key2 == NULL) {
        return 0;
    } else if (key1 == NULL) {
        return -1;
    } else if (key2 == NULL) {
        return 1;
    }
    Result* key2Result = (Result*) key2Value;
    char key1Op = key_get_op(key1);
    char key2Op = key_get_op(key2);
    if (IS_AND(key1Op)) {
        if (IS_AND(key2Op)) {
            return (strcmp(key_get_term1(key1), key_get_term1(key2)) == 0 && strcmp(key_get_term2(key1), key_get_term2(key2)) == 0)
                || (strcmp(key_get_term1(key1), key_get_term2(key2)) == 0 && strcmp(key_get_term2(key1), key_get_term1(key2)) == 0) ? 0 : 1;
        }
        return 1;
    } else if (IS_OR(key1Op)) {
        if (IS_AND(key2Op) && partialCacheMatches == 2) {
            return strcmp(key_get_term1(key1), key_get_term1(key2)) == 0
                || strcmp(key_get_term1(key1), key_get_term2(key2)) == 0
                || strcmp(key_get_term2(key1), key_get_term1(key2)) == 0
                || strcmp(key_get_term2(key1), key_get_term2(key2)) == 0 ? 0 : 1;
        } else if (IS_OR(key2Op)) {
            if (partialCacheMatches == 2) {
                return ((strcmp(key_get_term1(key1), key_get_term1(key2)) == 0 || strcmp(key_get_term2(key1), key_get_term1(key2)) == 0) && key2Result->term1Found)
                    || ((strcmp(key_get_term1(key1), key_get_term2(key2)) == 0 || strcmp(key_get_term2(key1), key_get_term2(key2)) == 0) && key2Result->term2Found) ? 0 : 1;
            }
            return (strcmp(key_get_term1(key1), key_get_term1(key2)) == 0 && strcmp(key_get_term2(key1), key_get_term2(key2)) == 0)
                || (strcmp(key_get_term1(key1), key_get_term2(key2)) == 0 && strcmp(key_get_term2(key1), key_get_term1(key2)) == 0) ? 0 : 1;
        } else  if (IS_SINGLE(key2Op) && partialCacheMatches == 2) {
            return strcmp(key_get_term1(key1), key_get_term1(key2)) == 0 || strcmp(key_get_term2(key1), key_get_term1(key2)) == 0 ? 0 : 1;
        }
        return 1;
    } else if (IS_SINGLE(key1Op)) {
        if (IS_AND(key2Op) && partialCacheMatches == 2) {
            return strcmp(key_get_term1(key1), key_get_term1(key2)) == 0 || strcmp(key_get_term1(key1), key_get_term2(key2)) == 0 ? 0 : 1;
        } else if (IS_OR(key2Op) && partialCacheMatches >= 1) {
            return (strcmp(key_get_term1(key1), key_get_term1(key2)) == 0 && key2Result->term1Found)
                || (strcmp(key_get_term1(key1), key_get_term2(key2)) == 0 && key2Result->term2Found) ? 0 : 1;
        } else if (IS_SINGLE(key2Op)) {
            return strcmp(key_get_term1(key1), key_get_term1(key2));
        }
        return 1;
    }
    return 1;
}


static void locked_dqht_print_table(Cache* cache, char* prefix, DequeueHashTable* dqht) {
    if (htfh_rwlock_wrlock_handled(&cache->rwlock) != 0) {
        return;
    }
    dqht_print_table(prefix, dqht);
    htfh_rwlock_unlock_handled(&cache->rwlock);
}

static pthread_barrier_t barrier;

#define ENTRY_COUNT 150

char* to_store[10];
PostIt postings[ENTRY_COUNT];
Result values[ENTRY_COUNT];

void createTestData() {
    to_store[0] = key_create('^', "test", "other");
    to_store[1] = key_create('^', "more", "something");
    to_store[2] = key_create('^', "bested", "test");
    to_store[3] = key_create('^', "caused", "cheese");
    to_store[4] = key_create('^', "vert", "based");
    to_store[5] = key_create('^', "mother", "lurch");
    to_store[6] = key_create('^', "tube", "deafen");
    to_store[7] = key_create('^', "loud", "clamp");
    to_store[8] = key_create('^', "boast", "cordial");
    to_store[9] = key_create('^', "nuke", "plate");
    for (int i = 0; i < ENTRY_COUNT; i++) {
        postings[i] = (PostIt) {
            .dn = i,
            .wc = rand() % 300
        };
    }
    for (int i = 0; i < ENTRY_COUNT; i++) {
        values[i] = (Result) {
            .term1Found = true,
            .term2Found = true,
            .offset2 = 0,
            .match_count = 10,
            .results = postings
        };
    }
}

void destroyTestData() {
    for (int i = 0; i < 10; i++) {
        free(to_store[i]);
    }
}

typedef struct Params {
    int index;
    Cache* cache;
} Params;

void* threadFn(void* arg) {
    Params* params = (Params*) arg;
    Cache* cache = (Cache*) params->cache;
    LOCALISE_ALLOCATOR_ARG
    int index = params->index;
    DEBUG("======== BEFORE WAITING ========");
    pthread_barrier_wait(&barrier);
    DEBUG("======== AFTER WAITING ========");
    INFO("======== REQUEST 1 ========");
    locked_dqht_print_table(cache, "Non-Resident HIRS", cache->backing->non_resident_hirs);
    locked_dqht_print_table(cache, "LIRS", cache->backing->lirs);
    locked_dqht_print_table(cache, "Resident HIRS", cache->backing->resident_hirs);
    for (int i = 0; i < ENTRY_COUNT; i++) {
        for (int j = 1; j < i + 1; j++) {
            DLIRSEntry* match = NULL;
            DLIRSEntry* evicted = NULL;
            int requestResult;
            INFO("==== THREAD: %d ====\n", index);
            if ((match = cache_get(cache, to_store[i % 10], (void**) &evicted)) != NULL) {
                continue;
            }
            if (evicted != NULL) {
                am_free(((Result*) evicted->value)->results);
                am_free(evicted->value);
                dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
            }
            if ((requestResult = cache_request(cache, to_store[i % 10], result_copy(AM_ALLOCATOR_ARG &values[i]), (void**) &evicted)) == -1) {
                INFO("[%d:%d] Unable to request cache population for [%s: %p] [Evicted: %p]", i, j, key_sprint(to_store[i % 10]), &values[i], evicted);
                am_free(((Result*) evicted->value)->results);
                am_free(evicted->value);
                dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
                cache_destroy(cache);
                pthread_kill(pthread_self(), 1);
                return NULL;
            }
            INFO("[%d:%d] Request result: %s with evicted: %p for [%s: %p]", i, j, requestResult == 1 ? "hit" : "miss", evicted, key_sprint(to_store[i % 10]), &values[i]);
            locked_dqht_print_table(cache, "Non-Resident HIRS", cache->backing->non_resident_hirs);
            locked_dqht_print_table(cache, "LIRS", cache->backing->lirs);
            locked_dqht_print_table(cache, "Resident HIRS", cache->backing->resident_hirs);
            if (evicted != NULL) {
                am_free(((Result*) evicted->value)->results);
                am_free(evicted->value);
                dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
            }
            TRACE("Cache is full? %s %d", cache_is_full(cache) ? "true" : "false", i);
        }
    }
    INFO("======== CONTAINS ========");
    locked_dqht_print_table(cache, "Non-Resident HIRS", cache->backing->non_resident_hirs);
    locked_dqht_print_table(cache, "LIRS", cache->backing->lirs);
    locked_dqht_print_table(cache, "Resident HIRS", cache->backing->resident_hirs);
    for (int i = 0; i < 10; i++) {
        INFO("==== THREAD: %d ====", index);
        DLIRSEntry* evicted = NULL;
        void* match = cache_get(cache, to_store[i % 10], (void**) &evicted);
        if (evicted != NULL) {
            am_free(((Result*) evicted->value)->results);
            am_free(evicted->value);
            dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
        }
        INFO("Cache contains %s: %s [Value: %p]", key_sprint(to_store[i]), match != NULL ? "true" : "false", match);
    }
    INFO("======== REQUEST 2 ========");
    for (int i = 0; i < ENTRY_COUNT; i++) {
        DLIRSEntry* evicted;
        DLIRSEntry* match;
        int requestResult;
        INFO("==== THREAD: %d ====", index);
        if ((match = cache_get(cache, to_store[i % 10], (void**) &evicted)) != NULL) {
            continue;
        }
        if (evicted != NULL) {
            am_free(((Result*) evicted->value)->results);
            am_free(evicted->value);
            dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
        }
        if ((requestResult = cache_request(cache, to_store[i % 10], result_copy(AM_ALLOCATOR_ARG &values[i]), (void**) &evicted)) == -1) {
            INFO("Unable to request cache population for [%s: %p] [Evicted: %p]", key_sprint(to_store[i % 10]), &values[i], evicted);
            am_free(((Result*) evicted->value)->results);
            am_free(evicted->value);
            dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
            cache_destroy(cache);
            pthread_kill(pthread_self(), 1);
            return NULL;
        }
        INFO("[%d] Request result: %s with evicted: %p for [%s: %p]", i, requestResult == 1 ? "hit" : "miss", evicted, key_sprint(to_store[i]), &values[i]);
        locked_dqht_print_table(cache, "Non-Resident HIRS", cache->backing->non_resident_hirs);
        locked_dqht_print_table(cache, "LIRS", cache->backing->lirs);
        locked_dqht_print_table(cache, "Resident HIRS", cache->backing->resident_hirs);
        if (evicted != NULL) {
            am_free(((Result*) evicted->value)->results);
            am_free(evicted->value);
            dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
        }
        TRACE("Cache is full? %s %d", cache_is_full(cache) ? "true" : "false", i);
    }
    INFO("<><><><> END OF TEST <><><><>");
    pthread_exit(0);
}

int main(int argc, char* argv[]) {
    srand(time(0));
    createTestData();
    Cache* cache = cache_create(
        HEAP_SIZE,
        10,
        4,
        DLIRS_CACHE_BACKING_HANDLERS,
        &(DLIRSOptions) {
            .hirs_ratio = 0.01f,
            .value_copy_handler = (ValueCopy) result_copy,
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
    destroyTestData();
    return 0;
}