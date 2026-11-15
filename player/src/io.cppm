export module qcm.player.io;
export import rstd;
export import wavsen.audio;

using namespace rstd::prelude;
namespace wa = wavsen::audio;

export namespace qcm::player_process
{
struct MediaSource {
    String                        uri;
    Option<Box<dyn<wa::MediaIo>>> io;
    bool                          local {};
};

auto parse_source(ref<str>) -> Result<MediaSource, String>;
} // namespace qcm::player_process
