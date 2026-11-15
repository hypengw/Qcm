export module qcm.player:metadata;
export import rstd;
using rstd::collections::BTreeMap;
using rstd::path::Path;
using namespace rstd::prelude;

export namespace player
{
struct Metadata {
    struct Stream {
        i64    bitrate;
        String mime;
    };
    BTreeMap<String, String> tags;
    Vec<Stream>              streams;
};

auto get_metadata(ref<Path>) -> Metadata;

} // namespace player
