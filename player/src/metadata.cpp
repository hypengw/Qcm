module qcm.player;
import :metadata;
import rstd;
import wavsen.audio;
using rstd::path::Path;
using namespace rstd::prelude;

namespace player
{
auto get_metadata(ref<Path> path) -> Metadata {
    auto url = path.to_str();
    if (url.is_none()) return {};
    auto opened = wavsen::audio::OpenedMedia::open(url.unwrap());
    if (opened.is_err()) return {};
    auto        media = rstd::move(opened).unwrap();
    const auto& info  = media.info();
    Metadata    result;
    for (const auto& tag : info.tags) result.tags.insert(tag.key.clone(), tag.value.clone());
    for (const auto& stream : info.streams)
        result.streams.push(Metadata::Stream { stream.bitrate, stream.mime.clone() });
    return result;
}
} // namespace player
