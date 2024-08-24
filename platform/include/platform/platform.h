#pragma once

#include <cstdint>

namespace plt
{

void        malloc_init();
std::size_t malloc_trim(std::size_t pad);
void        malloc_trim_count(std::size_t pad, std::size_t count);

struct MemInfo {
    std::size_t totle_in_use;
    std::size_t heap;
    std::size_t mmap;
    std::size_t mmap_num;
};

auto mem_info() -> MemInfo;

} // namespace plt