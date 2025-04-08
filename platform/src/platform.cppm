module;
#include <cstdint>
#include <cstddef>
export module platform;

export namespace plt
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

void set_thread_name(const char* name);

auto is_terminal() -> bool;

} // namespace plt