#pragma once

#ifndef _C_HTFH_RT_SEARCH_CACHE_MATH_UTILS_
#define _C_HTFH_RT_SEARCH_CACHE_MATH_UTILS_

#ifdef __cplusplus
extern "C" {
#endif

#define math_min(a,b) ({ \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b; \
})
#define math_max(a,b) ({ \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b; \
})

#ifdef __cplusplus
};
#endif

#endif // _C_HTFH_RT_SEARCH_CACHE_MATH_UTILS_