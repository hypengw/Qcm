module;
#include <rstd/macro.hpp>
export module qcm.backend:player.preferences;
export import rstd;
import rstd.json;
import rstd.serde;

using rstd::path::Path;
using namespace rstd::prelude;
using namespace rstd::literals;

namespace qcm::backend
{
struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", ignore_unknown)]]
PlayerPreferencesData {
    [[pmacro::helper("serde::derive", default)]] f64  volume { 1 };
    [[pmacro::helper("serde::derive", default)]] u32  fade_ms { 500 };
    [[pmacro::helper("serde::derive", default)]] i32  loop_mode {};
    [[pmacro::helper("serde::derive", default)]] bool shuffle {}, random_mode {};
};
} // namespace qcm::backend

export namespace qcm::backend
{
struct PlayerPreferences {
    double volume { 1 };
    u32    fade_ms { 500 };
    i32    loop_mode {};
    bool   shuffle {}, random_mode {};

    auto        operator==(const PlayerPreferences&) const -> bool = default;
    static auto load(ref<Path> file) -> Result<PlayerPreferences, String>;
    auto        save(ref<Path> file) const -> Result<empty, String>;

private:
    auto validate() const -> Result<empty, String> {
        if (! f64(volume).is_finite() || volume < 0 || volume > 1)
            return Err("player volume must be finite and between 0 and 1"_Str);
        if (fade_ms > u32::MAX / u32(1000))
            return Err("player fade duration exceeds the supported range"_Str);
        if (loop_mode < i32() || loop_mode > i32(2))
            return Err("player loop mode must be between 0 and 2"_Str);
        return Ok(empty {});
    }
};
} // namespace qcm::backend

namespace qcm::backend
{
auto PlayerPreferences::load(ref<Path> file) -> Result<PlayerPreferences, String> {
    auto input = rstd::fs::read_to_string(file);
    if (input.is_err()) {
        auto error = rstd::move(input).unwrap_err();
        if (error.kind() == rstd::io::ErrorKind { rstd::io::ErrorKind::NotFound })
            return Ok(PlayerPreferences {});
        return Err(rstd::format("cannot read player preferences '{}': {}", file, error));
    }
    auto decoded = rstd_try(rstd::json::decode_direct<PlayerPreferencesData>(input->as_str()),
                            rstd::format("cannot decode player preferences '{}': {}", file, _e));
    PlayerPreferences result { decoded.volume.to_primitive(),
                               decoded.fade_ms,
                               decoded.loop_mode,
                               decoded.shuffle,
                               decoded.random_mode };
    rstd_try(result.validate());
    return Ok(result);
}

auto PlayerPreferences::save(ref<Path> file) const -> Result<empty, String> {
    rstd_try(validate());
    auto output = rstd_try(rstd::json::encode(PlayerPreferencesData {
                               f64(volume), fade_ms, loop_mode, shuffle, random_mode }),
                           rstd::format("cannot encode player preferences '{}': {}", file, _e));
    if (auto parent = file.parent(); parent.is_some() && ! parent->as_os_str().is_empty())
        rstd_try(rstd::fs::create_dir_all(*parent),
                 rstd::format("cannot create player preferences directory '{}': {}", *parent, _e));
    rstd_try(rstd::fs::write_atomic(file, output.as_str().as_bytes()),
             rstd::format("cannot save player preferences '{}': {}", file, _e));
    return Ok(empty {});
}
} // namespace qcm::backend
