#include <stdlib.h>

#include "logging/logging.h"
//LOGS_DIR("/mnt/e/HTFH-RT-Search-Cache/logs");

#define cache_backing_t DLIRS*
#include "cache/cache.h"
#include "allocator/error/allocator_errno.h"

#define HEAP_SIZE (16 * 10000)

#define AND_OP '^'
#define OR_OP '|'
#define OR_OP_STR "|"

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

int main(int argc, char* argv[]) {
    srand(time(0));
    Cache* cache = cache_create(
        HEAP_SIZE,
        8,
        4,
        (CacheBackingHandlers) {
            .createHandler = (CacheBackingCreate) dlirs_create,
            .destroyHandler = dlirs_destroy,
            .containsHandler = dlirs_contains,
            .isFullHandler = dlirs_is_full,
            .requestHandler = (CacheBackingRequest) dlirs_request,
            .getHandler = dlirs_get
        },
        &(DLIRSOptions) {
            .hirs_ratio = 0.01f,
            .comparator = key_compare
        }
    );
    if (cache == NULL) {
        FATAL("Could not create cache with size 10");
    }
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
    INFO("======== REQUEST 1 ========");
    dqht_print_table("Init HIRS", cache->backing->hirs);
    dqht_print_table("Init LIRS", cache->backing->lirs);
    dqht_print_table("Init Q", cache->backing->q);
    for (int i = 0; i < 10; i++) {
        for (int j = 1; j < i + 1; j++) {
            DLIRSEntry* evicted = NULL;
            int requestResult;
            if ((requestResult = cache_request(cache, to_store[i], &values[i], (void**) &evicted)) == -1) {
                ERROR("[%d:%d] Unable to request cache population for [%s: %d] [Evicted: %p]", i, j, to_store[i], values[i], evicted);
                LOCALISE_ALLOCATOR_ARG
                dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
                cache_destroy(cache);
                return 1;
            }
            INFO("[%d:%d] Request result: %s with evicted: %p for [%s: %d]", i, j, requestResult == 1 ? "hit" : "miss", evicted, to_store[i], values[i]);
            dqht_print_table("HIRS", cache->backing->hirs);
            dqht_print_table("LIRS", cache->backing->lirs);
            dqht_print_table("Q", cache->backing->q);
            LOCALISE_ALLOCATOR_ARG
            dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
            TRACE("Cache is full? %s", cache_is_full(cache) ? "true" : "false");
        }
    }
    INFO("======== CONTAINS ========");
    dqht_print_table("HIRS", cache->backing->hirs);
    dqht_print_table("LIRS", cache->backing->lirs);
    dqht_print_table("Q", cache->backing->q);
    for (int i = 0; i < 10; i++) {
        void* match = cache_get(cache, to_store[i]);
        INFO("Cache contains %s: %s [Value: %p]", to_store[i], match != NULL ? "true" : "false", match);
    }
    INFO("======== REQUEST 2 ========");
    for (int i = 0; i < 10; i++) {
        DLIRSEntry* evicted;
        int requestResult;
        if ((requestResult = cache_request(cache, to_store[i], &values[i], (void**) &evicted)) == -1) {
            ERROR("Unable to request cache population for [%s: %d] [Evicted: %p]", to_store[i], values[i], evicted);
            LOCALISE_ALLOCATOR_ARG
            dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
            cache_destroy(cache);
            return 1;
        }
        INFO("[%d] Request result: %s with evicted: %p for [%s: %d]", i, requestResult == 1 ? "hit" : "miss", evicted, to_store[i], values[i]);
        dqht_print_table("HIRS", cache->backing->hirs);
        dqht_print_table("LIRS", cache->backing->lirs);
        dqht_print_table("Q", cache->backing->q);
        LOCALISE_ALLOCATOR_ARG
        dlirs_entry_destroy(AM_ALLOCATOR_ARG evicted);
        TRACE("Cache is full? %s", cache_is_full(cache) ? "true" : "false");
    }
    INFO("======== CLEANUP ========");
    dqht_print_table("HIRS", cache->backing->hirs);
    dqht_print_table("LIRS", cache->backing->lirs);
    dqht_print_table("Q", cache->backing->q);
    if (cache_destroy(cache) != 0) {
        FATAL("Could not destroy cache");
    }
    return 0;
}