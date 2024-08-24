#include "platform/platform.h"
#include <cstdio>
#include <atomic>
#include <cstdlib>

#if defined(__GLIBC__)
#    include <malloc.h>
#endif

#include "core/core.h"
#include "core/fmt.h"

namespace plt
{

void malloc_init() {
    // glibc fragmentation
    // https://github.com/prestodb/presto/issues/8993
#if defined(__GLIBC__)
    putenv((char*)"MALLOC_ARENA_MAX=2");
    mallopt(M_ARENA_MAX, 2);
#endif
}

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

auto mem_info() -> MemInfo {

    auto info = mallinfo2();
    return MemInfo {
        .totle_in_use = info.uordblks,
        .heap = info.arena,
        .mmap = info.hblkhd,
        .mmap_num = info.hblks,
    };

}

} // namespace plt