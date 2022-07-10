#define ENABLE_LOGGING
#include "logging/logging.h"

#include <stdlib.h>
#include "cache/dlirs/dlirs.h"
#include "allocator/error/allocator_errno.h"

#define print_error(subs, bytes) \
    char msg[100] \
    sprintf(msg, subs, bytes); \
    alloc_perror(msg); \
    return 1

#define HEAP_SIZE (16 * 10000)

int main(int argc, char* argv[]) {
    DLIRS* dlirs = dlirs_create(8, 4, 5, 0.1f);
    if (dlirs == NULL) {
        printf("Could not create table with size 10\n");
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
    dqht_print_table("Init HIRS", dlirs->hirs);
    dqht_print_table("Init LIRS", dlirs->lirs);
    dqht_print_table("Init Q", dlirs->q);
    for (int i = 0; i < 10; i++) {
        for (int j = 1; j < i + 1; j++) {
            DLIRSEntry* evicted = NULL;
            int requestResult;
            if ((requestResult = dlirs_request(dlirs, to_store[i], &values[i], &evicted)) == -1) {
                ERROR("[%d:%d] Unable to request cache population for [%s: %d] [Evicted: %p]", i, j, to_store[i], values[i], evicted);
                dlirs_destroy(dlirs);
                return 1;
            }
            INFO("[%d:%d] Request result: %s with evicted: %p for [%s: %d]", i, j, requestResult == 0 ? "hit" : "miss", evicted, to_store[i], values[i]);
            dqht_print_table("HIRS", dlirs->hirs);
            dqht_print_table("LIRS", dlirs->lirs);
            dqht_print_table("Q", dlirs->q);
        }
    }
    dqht_print_table("HIRS", dlirs->hirs);
    dqht_print_table("LIRS", dlirs->lirs);
    dqht_print_table("Q", dlirs->q);
    for (int i = 0; i < 10; i++) {
        INFO("Cache contains %s: %s", to_store[i], dlirs_contains(dlirs, to_store[i]) == 0 ? "true" : "false");
    }
    for (int i = 0; i < 10; i++) {
        DLIRSEntry* evicted;
        int requestResult;
        if ((requestResult = dlirs_request(dlirs, to_store[i], &values[i], &evicted)) == -1) {
            ERROR("Unable to request cache population for [%s: %d] [Evicted: %p]", to_store[i], values[i], evicted);
            dlirs_destroy(dlirs);
            return 1;
        }
        INFO("Request result: %s", requestResult == 0 ? "hit" : "miss");
    }
    dqht_print_table("HIRS", dlirs->hirs);
    dqht_print_table("LIRS", dlirs->lirs);
    dqht_print_table("Q", dlirs->q);
    dlirs_destroy(dlirs);
    return 0;
}