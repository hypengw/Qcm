export module qcm.backend:provider.playback;
export import rstd;
export import rstd.serde;
using namespace rstd::prelude;

export namespace qcm::backend
{
enum class [[pmacro::derive("serde::derive"),
             pmacro::helper("serde::derive", rename("PlaybackKind"), decode_only)]] PlaybackKind
{
    ByteResource [[pmacro::helper("serde::derive", rename("byte_resource"))]],
    HlsVod [[pmacro::helper("serde::derive", rename("hls_vod"))]]
};
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", rename("PlaybackRequest"), encode_only, ignore_unknown)]]
PlaybackRequest {
    String      item_id;
    Option<u32> max_bitrate;
};
struct [[pmacro::derive("serde::derive"),
         pmacro::helper("serde::derive", rename("PreparedPlayback"), decode_only, ignore_unknown)]]
PreparedPlayback {
    PlaybackKind kind { PlaybackKind::ByteResource };
    [[pmacro::helper("serde::derive", default)]]
    String uri;
    [[pmacro::helper("serde::derive", default)]]
    String representation;
    [[pmacro::helper("serde::derive", default)]]
    String state;
    [[pmacro::helper("serde::derive", default)]]
    Option<f64> duration_seconds;
    static auto byte_resource() -> PreparedPlayback;
};
inline auto PreparedPlayback::byte_resource() -> PreparedPlayback { return {}; }
} // namespace qcm::backend
