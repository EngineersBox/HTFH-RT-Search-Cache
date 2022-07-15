#pragma once

#ifndef HTFH_RT_SEARCH_CACHE_LAMBDA_H
#define HTFH_RT_SEARCH_CACHE_LAMBDA_H

#ifdef __cplusplus
extern "C" {
#endif

#define lambda(lambda$_ret, lambda$_args, lambda$_body) ({ \
    lambda$_ret lambda$__anon$ lambda$_args \
        lambda$_body \
    &lambda$__anon$; \
})

#ifdef __cplusplus
};
#endif

#endif //HTFH_RT_SEARCH_CACHE_LAMBDA_H
