#ifndef EASTL_USERCONFIG_H
#define EASTL_USERCONFIG_H

#include "assert.h"


#define EASTL_DEBUG 1

#define EA_COMPILER_NO_STANDARD_CPP_LIBRARY 1

#define EASTL_CPP11_MUTEX_ENABLED 1

#define EASTL_ALLOCATOR_COPY_ENABLED 1

#define EASTL_RTTI_ENABLED 0

#define EASTL_EXCEPTIONS_ENABLED 0

#define EASTL_INT128_SUPPORTED 0

#define EASTL_DEFAULT_ALLOCATOR_ALIGNED_ALLOCATIONS_SUPPORTED 0

#define EASTL_OPENSOURCE 1

#define EASTL_ASSERT assert

#define EASTL_ASSERT_MSG(expression, message) assert(expression && message)

#define EASTL_FAIL_MSG(message) assert(false && message)


extern void kprintfd(const char *fmt, ...);
[[maybe_unused]] const auto printf = kprintfd;

#endif
