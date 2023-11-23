#include "platform/platform.h"
#include <cstdio>
#include <atomic>

#if defined(__GLIBC__)
#    include <malloc.h>
#endif

namespace plt
{
std::size_t malloc_trim(std::size_t pad) {
#if defined(__GLIBC__)
    return ::malloc_trim(pad);
#else
    return 1;
#endif
}

void malloc_trim_count(std::size_t pad, std::size_t count) {
    static std::atomic<std::size_t> size { 0 };
    if (size = (size + 1) % count; size == 0) {
        malloc_trim(pad);
    }
}
} // namespace plt