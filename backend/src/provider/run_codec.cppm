export module qcm.backend:provider.run_codec;
export import :provider.run;
using namespace rstd::prelude;

export namespace qcm::backend::lua_run
{
auto sync_libraries(const luato::LuaValue& items, luato::DeserializeOptions options = {})
    -> Result<LuaRunCommand, luato::Error>;
auto sync_songs(const luato::LuaValue& items, luato::DeserializeOptions options = {})
    -> Result<LuaRunCommand, luato::Error>;
auto sync_images(const luato::LuaValue& items, luato::DeserializeOptions options = {})
    -> Result<LuaRunCommand, luato::Error>;
auto sync_artists(const luato::LuaValue& items, luato::DeserializeOptions options = {})
    -> Result<LuaRunCommand, luato::Error>;
auto sync_radio_queues(const luato::LuaValue& items, luato::DeserializeOptions options = {})
    -> Result<LuaRunCommand, luato::Error>;
auto sync_albums(const luato::LuaValue& items, LegacySyncOptions sync_options,
                 luato::DeserializeOptions options = {}) -> Result<LuaRunCommand, luato::Error>;
auto sync_dynamics(const luato::LuaValue& items, LegacySyncOptions sync_options,
                   luato::DeserializeOptions options = {}) -> Result<LuaRunCommand, luato::Error>;
auto sync_remote_mixes(const luato::LuaValue& items, LegacySyncOptions sync_options,
                       luato::DeserializeOptions options = {})
    -> Result<LuaRunCommand, luato::Error>;
auto sync_album_artist_ids(const luato::LuaValue& items, luato::DeserializeOptions options = {})
    -> Result<LuaRunCommand, luato::Error>;
auto sync_song_artist_ids(const luato::LuaValue& items, luato::DeserializeOptions options = {})
    -> Result<LuaRunCommand, luato::Error>;
auto allocate_items(const luato::LuaValue& items, i64 provider,
                    luato::DeserializeOptions options = {}) -> Result<LuaRunCommand, luato::Error>;
auto sync_remote_mix_song_ids(i64 mix, const luato::LuaValue& items,
                              luato::DeserializeOptions options = {})
    -> Result<LuaRunCommand, luato::Error>;
auto observe_items(ObserveScope scope, const luato::LuaValue& items,
                   luato::DeserializeOptions options = {}) -> Result<LuaRunCommand, luato::Error>;
auto resolve_items(ref<str> name, const luato::LuaValue& items,
                   luato::DeserializeOptions options = {}) -> Result<LuaRunCommand, luato::Error>;
} // namespace qcm::backend::lua_run
