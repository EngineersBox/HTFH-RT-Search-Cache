#pragma once

#ifndef C_HTFH_RT_SEARCH_CACHE_MATH_UTILS
#define C_HTFH_RT_SEARCH_CACHE_MATH_UTILS

#ifdef __cplusplus
extern "C" {
#endif

#define math_min(a,b) ((__typeof__(a))(a)) < ((__typeof__(b))(b)) ? ((__typeof__(a))(a)) : ((__typeof__(b))(b))
#define math_max(a,b) ((__typeof__(a))(a)) > ((__typeof__(b))(b)) ? ((__typeof__(a))(a)) : ((__typeof__(b))(b))

#ifdef __cplusplus
};
#endif

#endif // C_HTFH_RT_SEARCH_CACHE_MATH_UTILS