local lito = require("@lito")
local files = {}
for _, path in ipairs({
  "plugin.json", "jellyfin.svg", "src/provider.lua", "src/mapping.lua",
  "src/jellyfin_api/client.lua", "src/jellyfin_api/models.lua", "src/jellyfin_api/types.lua",
  "src/jellyfin_api/operations.lua", "src/jellyfin_api/runtime.lua",
}) do
  files[#files + 1] = { source = path, destination = "bin/plugins/jellyfin/" .. path }
end
for _, name in ipairs({
  "get_posted_playback_info", "ping_playback_session", "stop_encoding_process",
  "authenticate_user_by_name", "get_album_artists", "get_artists", "get_audio_stream",
  "get_endpoint_info", "get_item", "get_item_image", "get_items", "get_lyrics",
  "get_playlist_items", "get_user_views", "get_virtual_folders", "mark_favorite_item", "unmark_favorite_item",
}) do
  local path = "src/jellyfin_api/operations/" .. name .. ".lua"
  files[#files + 1] = { source = path, destination = "bin/plugins/jellyfin/" .. path }
end
lito.install({ files = files })
