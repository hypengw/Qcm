#pragma once
#include <cstdint>
#include <string>
#include <map>
#include <vector>

namespace player
{
struct Metadata {
    struct Stream {
        std::int64_t bitrate;
        std::string  mime;
    };
    std::map<std::string, std::string, std::less<>> tags;
    std::vector<Stream>                             streams;
};

} // namespace player