local map = require('mapping')
local resolve = require('resolver')
local J = qcm.json.strict
local M = {}
local id = 'random'
function M.entries()
    return {{native_id = id, name = '随机漫游', description = '从音乐库随机发现歌曲'}}
end
local function check(native) assert(native == id, 'unknown Subsonic radio') end
function M.definition(native)
    check(native)
    return {configuration = {}, can_restart = true, restart_label = '重新随机'}
end
function M.fetch(ctx, api, current)
    local seen, rows = {}, {}
    for _, native in ipairs(current) do seen[map.native(native)] = true end
    for _ = 1, 3 do
        local result = api:call('get_random_songs', {size = 20}, 'randomSongs')
        local candidates = map.value(result.song) or {}
        for _, song in ipairs(candidates) do
            local native = map.native(song.id)
            if not seen[native] and song.isVideo ~= true then
                seen[native] = true
                rows[#rows + 1] = song
            end
        end
        if #rows > 0 or #candidates == 0 then break end
    end
    return {songs = resolve.write(ctx, resolve.library(ctx), 'Song', rows),
        state = J.encode(J.object {version = 1}), configuration = {},
        presentation = {description = '从音乐库随机发现歌曲'}, exhausted = #rows == 0}
end
function M.open(ctx, api, native, request)
    check(native)
    assert(next(request.configuration or {}) == nil and request.input == nil, 'random radio does not accept configuration or input')
    return M.fetch(ctx, api, {})
end
function M.next(ctx, api, native, request)
    check(native)
    local state = J.decode(request.state)
    assert(state.version == 1, 'invalid random radio state')
    for key in pairs(state) do assert(key == 'version', 'invalid random radio state') end
    assert(#request.current_native_ids <= 500, 'radio window exceeds limit')
    return M.fetch(ctx, api, request.current_native_ids)
end
return M
