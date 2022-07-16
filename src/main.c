#define DQHT_ENABLE_STRICT
#define DLIRS_ENABLE_STRICT
#define HTFH_ALLOCATOR
#define ENABLE_LOGGING
#define LOG_DATETIME_PREFIX
#include "logging/logging.h"
LOGS_DIR("/mnt/e/HTFH-RT-Search-Cache/logs");

#include <stdlib.h>
#define cache_backing_t DLIRS*
#include "cache/cache.h"
#include "allocator/error/allocator_errno.h"

#define HEAP_SIZE (16 * 10000)

int main(int argc, char* argv[]) {
    typedef struct CacheOptions { float hirs_ratio; } CacheOptions;
    Cache* cache = cache_create(
        HEAP_SIZE,
        8,
        4,
        (CacheBackingHandlers) {
            .createHandler = (CacheBackingCreate) dlirs_create,
            .destroyHandler = dlirs_destroy,
            .containsHandler = dlirs_contains,
            .isFullHandler = dlirs_is_full,
            .requestHandler = (CacheBackingRequest) dlirs_request
        },
        &(CacheOptions) {
            .hirs_ratio = 0.01f
        }
    );
    if (cache == NULL) {
        printf("Could not create cache with size 10\n");
        return 1;
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
        INFO("Cache contains %s: %s", to_store[i], cache_contains(cache, to_store[i]) ? "true" : "false");
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
    cache_destroy(cache);
    return 0;
}