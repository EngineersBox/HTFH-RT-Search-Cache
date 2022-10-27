#pragma once


#ifndef HTFH_RT_SEARCH_CACHE_REFERENCE_COUNTER_H
#define HTFH_RT_SEARCH_CACHE_REFERENCE_COUNTER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef size_t __attribute__((__may_alias__)) ReferenceCounter;

#define rc_init(counter) (counter) = 1
#define rc_last(counter) ((counter) == 0)
#define rc_decrement(counter) (counter)--
#define rc_increment(counter) (counter)++

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_REFERENCE_COUNTER_H
