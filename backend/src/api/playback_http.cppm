export module qcm.backend:api.playback_http;
import :api.http;
import :error;
import liserveto;
import rstd;
import rstd.json;
using rstd::async::coro;
using rstd::sync::Arc;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct PlaybackSourceHttpRequest {
    i64         song_id;
    Option<i32> max_bitrate;
};

struct PlaybackResolveHttpRequest {
    String token;
    String uri;
};

struct PlaybackResourceHttpRequest {
    String token;
    usize  index;
    String path;
    String query;
    bool   head;
};

auto playback_body_json(liserveto::Request& request, bool allow_empty)
    -> coro<ProcessResult<rstd::json::Value>>;
auto playback_source_http_request(liserveto::Request& request)
    -> coro<ProcessResult<PlaybackSourceHttpRequest>>;
auto playback_resolve_http_request(liserveto::Request& request)
    -> coro<ProcessResult<PlaybackResolveHttpRequest>>;
auto playback_resource_http_request(const liserveto::Request& request)
    -> ProcessResult<PlaybackResourceHttpRequest>;
auto install_playback_http_routes(liserveto::Router& router, Arc<QcmService> service) -> void;
} // namespace qcm::backend
