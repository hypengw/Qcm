export module qcm.player:metadata;
import rstd.cppstd;

export namespace player
{
struct Metadata {
    struct Stream {
        std::int64_t bitrate;
        std::string  mime;
    };
    std::map<std::string, std::string, std::less<>> tags;
    std::vector<Stream>                             streams;
};

auto get_metadata(const std::filesystem::path&) -> Metadata;

} // namespace player
