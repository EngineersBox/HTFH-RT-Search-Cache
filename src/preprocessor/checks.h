#pragma once

#ifndef _C_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_CHECKS_
#define _C_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_CHECKS_

#if defined (__alpha__) || defined (__ia64__) || defined (__x86_64__) || defined (_WIN64) || defined (__LP64__) || defined (__LLP64__)
#define ARCH_64_BIT
#endif

#define init_check(type, var, init, comparison) type var; if ((var = init) comparison)

#endif // _C_HYBRID_TLSF_FIXED_HEAP_ALLOCATOR_CHECKS_