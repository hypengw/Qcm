local Client = require('client')
local map = require('mapping')
local sync = require('sync')
local Radio = require('radio')
local J = qcm.json.strict
local M = {}
local api, generation = nil, 0
local ready = false
local function client(base, method)
    local revision = generation
    return Client.new(base, method, function() return generation == revision end)
end
local function require_api()
    if not api then qcm.error {code = 'not_auth'} end
    local current = api
    if not ready then
        qcm.inner:session_task('subsonic:discovery', function() current:discover(); return true end)
        current:check()
        ready = true
    end
    return current
end
function M.load(data)
    generation = generation + 1
    api, ready = nil, false
    if data ~= '' then assert(J.decode(data).version == 1, 'invalid saved Subsonic state') end
    local method = qcm.inner.auth_method
    if method then api = client(qcm.inner.base_url, method) end
end
function M.save() return J.encode(J.object {version = 1}) end
function M.resource_scope()
    if not api then qcm.error {code = 'not_auth'} end
    return J.encode(J.object {version = 1, username = api.username})
end
function M.login(info)
    generation = generation + 1
    api, ready = nil, false
    local pending = client(info.server_url, info.method)
    pending:discover()
    api, ready = pending, true
    return {type = 'Ok'}
end
function M.check()
    if not ready then require_api() else require_api():call('ping', {}) end
end
function M.sync(ctx) sync.full(ctx, require_api()) end
function M.sync_item(ctx, item) sync.item(ctx, require_api(), item) end
function M.favorite(ctx, id, kind, favorite)
    local fields = {[qcm.enum.ItemType.Song] = 'id', [qcm.enum.ItemType.Album] = 'album_id', [qcm.enum.ItemType.Artist] = 'artist_id'}
    local field = assert(fields[kind], 'unsupported favorite type')
    require_api():call(favorite and 'star' or 'unstar', {[field] = {map.native(id)}})
end
function M.prepare_audio(id)
    local request, validate = require_api():media('stream', {id = map.native(id), format = 'raw'})
    return qcm.resource.prepare(request, validate, {transient_query = {'t', 's'}})
end
function M.prepare_image(id, image_id, kind)
    assert(kind == 'Primary', 'unsupported image type')
    if not image_id then qcm.error {code = 'not_found'} end
    local request, validate = require_api():media('get_cover_art', {id = map.native(image_id), size = 512})
    return qcm.resource.prepare(request, validate, {transient_query = {'t', 's'}})
end
function M.audio(id, headers)
    return require_api():read_media('stream', {id = map.native(id), format = 'raw'}, headers)
end
function M.image(id, image_id, kind)
    assert(kind == 'Primary', 'unsupported image type')
    if not image_id then qcm.error {code = 'not_found'} end
    return require_api():read_media('get_cover_art', {id = map.native(image_id), size = 512})
end
function M.subtitle(id)
    local current = require_api()
    local items = {}
    if current.extensions.songLyrics and current.extensions.songLyrics[1] then
        local response = current:call('get_lyrics_by_song_id', {id = map.native(id)}, 'lyricsList')
        for _, lyrics in ipairs(map.value(response.structuredLyrics) or {}) do
            if lyrics.kind == nil or lyrics.kind == 'main' then
                for _, line in ipairs(map.value(lyrics.line) or {}) do
                    local start
                    if lyrics.synced then
                        assert(type(line.start) == 'number', 'missing lyric start')
                        start = math.max(0, math.floor(line.start - (lyrics.offset or 0)))
                    end
                    items[#items + 1] = {start = start, text = map.value(line.value)}
                end
                if #items > 0 then break end
            end
        end
    else
        local song = current:call('get_song', {id = map.native(id)}, 'song')
        local response = current:call('get_lyrics', {artist = map.value(song.artist), title = song.title}, 'lyrics')
        if map.value(response.value) and response.value ~= '' then items[1] = {text = response.value} end
    end
    return {items = items}
end
function M.queue_definition(ctx, native) return Radio.definition(native) end
function M.open_queue(ctx, native, request) return Radio.open(ctx, require_api(), native, request) end
function M.next_queue(ctx, native, request) return Radio.next(ctx, require_api(), native, request) end
function M.for_you_items(ctx)
    local entry = Radio.entries()[1]
    return {{entry_key = entry.native_id, target = map.key(nil, 'Radio', entry.native_id),
        presentation = {style = 'Radio', cover = {type = 'RadioCurrent'}},
        primary_action = 'ActivateRadio', play_action = 'ActivateRadio'}}
end
for name, callback in pairs(M) do
    M[name] = function(...)
        local ok, result = pcall(callback, ...)
        if ok then return result end
        Client.raise(result)
    end
end
return M
