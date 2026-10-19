local Client = require('client')
local map = require('mapping')
local resolve = require('resolver')
local Radio = require('radio')
local J = qcm.json.strict
local M = {}
local kinds = {'Artist', 'Album', 'Song'}
local function sets() return {Artist = {}, Album = {}, Song = {}} end
local function record(seen, kind, rows)
    for _, row in ipairs(rows) do
        local id = map.native(row.id)
        assert(not seen[kind][id], 'duplicate item in Subsonic enumeration')
        seen[kind][id] = true
    end
end
local function pages(fetch, consume, seed)
    local offset, seen = 0, {}
    for _ = 1, 5001 do
        local rows = seed or fetch(offset)
        seed = nil
        assert(type(rows) == 'table', 'missing Subsonic page')
        if #rows == 0 then return end
        for _, row in ipairs(rows) do
            local id = map.native(row.id)
            assert(not seen[id], 'Subsonic pagination made no progress')
            seen[id] = true
        end
        consume(rows)
        offset = offset + #rows
        assert(offset <= 200000, 'Subsonic synchronization limit exceeded')
    end
    error('Subsonic page limit exceeded', 0)
end
local function search(api, kind, offset)
    local params = {query = '', artist_count = 0, album_count = 0, song_count = 0}
    local group = kind:lower()
    params[group .. '_count'], params[group .. '_offset'] = 40, offset
    local response = api:call('search3', params, 'searchResult3')
    return map.value(response[group]) or {}
end
local function enumerate(api, modern, consume, seed)
    local seen = sets()
    local function accept(kind, rows)
        record(seen, kind, rows)
        consume(kind, rows)
    end
    if modern then
        for _, kind in ipairs(kinds) do
            pages(function(offset) return search(api, kind, offset) end,
                function(rows) accept(kind, rows) end, kind == 'Artist' and seed or nil)
        end
    else
        local artists = api:call('get_artists', {}, 'artists')
        for _, index in ipairs(map.value(artists.index) or {}) do accept('Artist', map.value(index.artist) or {}) end
        pages(function(offset)
            return map.value(api:call('get_album_list2', {type = 'alphabeticalByName', size = 40, offset = offset}, 'albumList2').album) or {}
        end, function(albums)
            for _, summary in ipairs(albums) do
                local album = api:call('get_album', {id = summary.id}, 'album')
                assert(album.id == summary.id, 'album identity mismatch')
                local songs = map.value(album.song) or {}
                if album.songCount ~= nil then assert(album.songCount == #songs, 'incomplete album songs') end
                accept('Album', {album})
                accept('Song', songs)
            end
        end)
    end
    return seen
end
local function equal(a, b)
    for key in pairs(a) do if not b[key] then return false end end
    for key in pairs(b) do if not a[key] then return false end end
    return true
end
local function playlists(api)
    local rows = map.value(api:call('get_playlists', {}, 'playlists').playlist) or {}
    local seen = {}
    for _, row in ipairs(rows) do
        local id = map.native(row.id)
        assert(not seen[id], 'duplicate playlist identity')
        seen[id] = true
    end
    return rows
end
function M.mix(row)
    return {item = map.key(nil, 'Mix', row.id), name = assert(row.name, 'missing playlist name'),
        description = map.value(row.comment), track_count = row.songCount or 0, mix_type = 'user'}
end
function M.full(ctx, api)
    local start = api:scan_status()
    local folders = map.value(api:call('get_music_folders', {}, 'musicFolders').musicFolder) or {}
    local modern, seed = api.open_subsonic, nil
    if modern then
        local ok, result = pcall(search, api, 'Artist', 0)
        local empty_query_unsupported = type(result) == 'table' and result.domain == 'subsonic' and result.code == 10
        if not ok and not Client.unsupported(result) and not empty_query_unsupported then error(result, 0) end
        modern, seed = ok, ok and result or nil
    end
    local library = ctx:sync_batch {libraries = {{native_id = 'music',
        name = #folders == 1 and folders[1].name or 'Music Library'}}}.libraries[1].library_id
    local scopes = {resolve.scope(nil, 'Mix'), resolve.scope(nil, 'Radio')}
    for _, kind in ipairs(kinds) do scopes[#scopes + 1] = resolve.scope(library, kind) end
    local run = ctx:begin_sync {libraries = {library}, scopes = scopes}
    run:complete('libraries')
    local observed = sets()
    local enumerated = enumerate(api, modern, function(kind, rows)
        resolve.write(ctx, library, kind, rows, run, observed)
    end, seed)
    local starred = api:call('get_starred2', {}, 'starred2')
    for _, kind in ipairs(kinds) do
        local favorites, inputs = {}, {}
        for _, row in ipairs(map.value(starred[kind:lower()]) or {}) do favorites[map.native(row.id)] = true end
        for id, key in pairs(observed[kind]) do
            inputs[#inputs + 1] = {item = key, favorite = favorites[id] == true}
            if #inputs == 200 then run:sync_batch {dynamics = inputs}; inputs = {} end
        end
        if #inputs > 0 then run:sync_batch {dynamics = inputs} end
    end
    for _, row in ipairs(playlists(api)) do
        local mix = M.mix(row)
        local images = {}
        if map.value(row.coverArt) then images[1] = {item_id = mix.item, image_type = 'Primary', native_id = row.coverArt, fresh = row.coverArt} end
        run:sync_batch {remote_mixes = {mix}, images = images}
        run:observe_items(resolve.scope(nil, 'Mix'), {mix.item})
    end
    local radios, keys = {}, {}
    for _, entry in ipairs(Radio.entries()) do
        local key = map.key(nil, 'Radio', entry.native_id)
        keys[#keys + 1] = key
        radios[#radios + 1] = {item = key, name = entry.name, description = entry.description, track_count = 0, mix_type = 'queue'}
    end
    run:sync_batch {remote_mixes = radios}
    run:observe_items(resolve.scope(nil, 'Radio'), keys)
    local finish = api:scan_status()
    local watermark = start and finish and map.value(start.lastScan) and map.value(finish.lastScan)
    if watermark then
        assert(start.lastScan == finish.lastScan and start.count == finish.count, 'server changed during synchronization')
    end
    if modern or not watermark then
        local confirmation = enumerate(api, modern, function() end)
        for _, kind in ipairs(kinds) do assert(equal(enumerated[kind], confirmation[kind]), 'library changed during synchronization') end
    end
    for _, kind in ipairs(kinds) do run:complete(resolve.scope(library, kind)) end
    run:complete(resolve.scope(nil, 'Mix'))
    run:complete(resolve.scope(nil, 'Radio'))
    run:seal()
end

function M.item(ctx, api, item)
    local kind = item.type
    local library = resolve.library(ctx)
    if kind == 'Mix' then
        local row = api:call('get_playlist', {id = item.native_id}, 'playlist')
        assert(row.id == item.native_id, 'playlist identity mismatch')
        local rows, ids, seen = map.value(row.entry) or {}, {}, {}
        assert(#rows <= 10000, 'playlist exceeds limit')
        if row.songCount ~= nil then assert(row.songCount == #rows, 'incomplete playlist') end
        for _, song in ipairs(rows) do
            local id = map.native(song.id)
            assert(not seen[id], 'repeated playlist songs are not supported')
            seen[id] = true
        end
        local songs = resolve.write(ctx, library, 'Song', rows)
        for _, song in ipairs(songs) do ids[#ids + 1] = song.id end
        local mix = M.mix(row)
        ctx:sync_batch {remote_mixes = {mix}, remote_mix_contents = {{mix = item.id, mode = 'replace', songs = ids}}}
    else
        assert(kind == 'Artist' or kind == 'Album' or kind == 'Song', 'unsupported item type')
        local row = api:call('get_' .. kind:lower(), {id = item.native_id}, kind:lower())
        assert(row.id == item.native_id, 'media identity mismatch')
        resolve.write(ctx, library, kind, {row})
        if kind == 'Album' then resolve.write(ctx, library, 'Song', map.value(row.song) or {}) end
        ctx:sync_batch {dynamics = {{item = item.id, favorite = map.value(row.starred) ~= nil}}}
    end
end
return M
