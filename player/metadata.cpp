module qcm.player;
import :metadata;
import rstd;
import rstd.cppstd;
import wavsen.audio;

namespace player
{
auto get_metadata(const std::filesystem::path& path) -> Metadata {
    const auto name = path.string();
    auto       url  = rstd::cppstd::as_str(name);
    if (url.is_err()) return {};
    auto opened = wavsen::audio::OpenedMedia::open(url.unwrap());
    if (opened.is_err()) return {};
    auto        media = rstd::move(opened).unwrap();
    const auto& info  = media.info();
    Metadata    result;
    auto        text = [](const auto& value) {
        return std::string(reinterpret_cast<const char*>(value.data()),
                           value.size().to_primitive());
    };
    for (const auto& tag : info.tags) result.tags.insert_or_assign(text(tag.key), text(tag.value));
    for (const auto& stream : info.streams)
        result.streams.push_back({ stream.bitrate.to_primitive(), text(stream.mime) });
    return result;
}
} // namespace player
