#pragma once

#include <cstdint>

namespace plt
{

void        malloc_init();
std::size_t malloc_trim(std::size_t pad);
void        malloc_trim_count(std::size_t pad, std::size_t count);

} // namespace plt