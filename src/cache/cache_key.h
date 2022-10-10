#pragma once

#ifndef LSIP_CACHE_KEY_H
#define LSIP_CACHE_KEY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../allocator/alloc_manager.h"

#define SINGLE_OP '0'

static char* key_create(char op, const char* term1, const char* term2) {
    size_t t1Size = (strlen(term1) + 1);
    size_t t2Size = term2 == NULL ? 0 : (strlen(term2) + 1);
    char* key = (char*) malloc(sizeof(char) + (2 * sizeof(size_t)) + ((t1Size + t2Size) * sizeof(char)));
    size_t offset = 0;
    key[offset] = op;
    offset += sizeof(char);
    memcpy(key + offset, &t1Size, sizeof(size_t));
    offset += sizeof(size_t);
    memcpy(key + offset, &t2Size, sizeof(size_t));
    offset += sizeof(size_t);
    memcpy(key + offset, term1, t1Size * sizeof(char));
    if (term2 != NULL) {
        offset += t1Size * sizeof(char);
        memcpy(key + offset, term2, t2Size * sizeof(char));
    }
    return key;
}

static size_t key_get_length(const char* keyStruct) {
    size_t length = sizeof(char) + (2 * sizeof(size_t)) + (size_t) (*(keyStruct + sizeof(char)));
    return keyStruct[0] == SINGLE_OP ? length : length + (size_t) (*(keyStruct + sizeof(char) + sizeof(size_t)));
}

static char key_get_op(const char* keyStruct) {
    return keyStruct[0];
}

static const char* key_get_term1(const char* keyStruct) {
    return keyStruct + sizeof(char) + (2 * sizeof(size_t));
}

static const char* key_get_term2(const char* keyStruct) {
    size_t t1Size = (size_t) (*(keyStruct + sizeof(char)));
    return keyStruct + sizeof(char) + (2 * sizeof(size_t)) + (t1Size * sizeof(char));
}

static size_t key_cmp(const char* keyStruct1, const char* keyStruct2) {
    char op1 = key_get_op(keyStruct1);
    return op1 == key_get_op(keyStruct2)
        && strcmp(key_get_term1(keyStruct1), key_get_term1(keyStruct2)) == 0
        && (op1 == SINGLE_OP || strcmp(key_get_term2(keyStruct1), key_get_term2(keyStruct2)) == 0) ? 0 : 1;
}

static size_t key_size(const char* keyStruct) {
    size_t length = sizeof(char) + (2 * sizeof(size_t)) + (((size_t) *(keyStruct + sizeof(char))) * sizeof(char));
    return keyStruct[0] == SINGLE_OP ? length : length + (((size_t) *(keyStruct + sizeof(char) + sizeof(size_t))) * sizeof(char));
}

static char* key_sprint(const char* keyStruct) {
    if (keyStruct == NULL) {
        return NULL;
    }
    if (keyStruct[0] == SINGLE_OP) {
        return (char*) key_get_term1(keyStruct);
    }
    char* formattedkey = (char*) malloc(sizeof(char) * 100);
    sprintf(formattedkey, "%s%c%s", key_get_term1(keyStruct), key_get_op(keyStruct), key_get_term2(keyStruct));
    return formattedkey;
}

static char* key_copy(AM_ALLOCATOR_PARAM char* key) {
    return key_create(key_get_op(key), key_get_term1(key), key_get_term2(key));
}

#ifdef __cplusplus
};
#endif

#endif //LSIP_CACHE_KEY_H
