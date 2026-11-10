local J = qcm.json.strict
local M = {}
M.__index = M

function M.value(value) if value ~= J.null then return value end end
local value = M.value
function M.native(id)
    assert(type(id) == 'string' and #id > 0 and #id <= 1024, 'invalid Subsonic ID')
    return id
end
function M.key(library, kind, id)
    return {library_id = library or J.null, type = kind, native_id = M.native(id)}
end
function M.new(library)
    return setmetatable({library = library, entries = {}, relations = {}, keys = {Artist = {}, Album = {}, Song = {}},
        data = {artists = {}, albums = {}, songs = {}, dynamics = {}, images = {}, album_artists = {}, song_artists = {}}}, M)
end
function M:relation(kind, owner, targets, mode)
    local identity = kind .. '\0' .. owner.native_id
    local relation = self.relations[identity]
    if not relation then
        relation = {owner = owner, mode = mode, targets = {}}
        self.relations[identity] = relation
        local group = kind == 'Album' and self.data.album_artists or self.data.song_artists
        group[#group + 1] = relation
    elseif mode == 'replace' then
        relation.mode, relation.targets = mode, {}
    end
    local seen = {}
    for _, target in ipairs(relation.targets) do seen[target.native_id] = true end
    for _, target in ipairs(targets) do
        if not seen[target.native_id] then
            seen[target.native_id] = true
            relation.targets[#relation.targets + 1] = target
        end
    end
end
local groups = {Artist = 'artists', Album = 'albums', Song = 'songs'}
local function copy(out, source, target, field, convert)
    local v = value(source[field])
    if v ~= nil then out[target] = convert and convert(v) or v end
end
local function duration(seconds)
    assert(type(seconds) == 'number' and seconds >= 0 and seconds < 1e10, 'invalid duration')
    return math.floor(seconds * 1000)
end
local function date(text) return qcm.time.parse_rfc3339(text) end
local function release(item)
    local d = value(item.releaseDate)
    if d and d.year and d.month and d.day then
        return date(('%04d-%02d-%02dT00:00:00Z'):format(d.year, d.month, d.day))
    end
end

function M:artists(item, field)
    local artists = value(item[field])
    if artists == nil and value(item.artistId) then artists = {{id = item.artistId, name = value(item.artist)}} end
    local keys, seen = {}, {}
    for _, artist in ipairs(artists or {}) do
        if value(artist.id) then
            local id = M.native(artist.id)
            if not seen[id] then
                seen[id] = true
                keys[#keys + 1] = self:add('Artist', {id = id, name = value(artist.name) or id}, true)
            end
        end
    end
    return keys, artists ~= nil
end

function M:add(kind, item, summary)
    local key = M.key(self.library, kind, item.id)
    local identity = kind .. '\0' .. key.native_id
    local previous = self.entries[identity]
    if previous and (summary or not previous.summary) then return key end
    local name = assert(value(item[kind == 'Song' and 'title' or 'name']), 'missing media name')
    assert(type(name) == 'string' and #name > 0, 'invalid media name')
    local out = {item = key}
    local fields = {name = name}
    if summary then out.on_insert = fields else
        out.name = name
        copy(out, item, 'sort_name', 'sortName')
        copy(out, item, 'added_at', 'created', date)
        if kind == 'Artist' then
            copy(out, item, 'album_count', 'albumCount')
        else
            copy(out, item, 'duration', 'duration', duration)
            out.publish_time = release(item)
            if kind == 'Album' then
                copy(out, item, 'track_count', 'songCount')
                out.on_insert = {disc_count = 1}
            else
                copy(out, item, 'track_number', 'track')
                copy(out, item, 'disc_number', 'discNumber')
                out.can_play = item.isVideo ~= true
                local genres = value(item.genres)
                if genres then
                    out.tags = {}
                    for _, genre in ipairs(genres) do out.tags[#out.tags + 1] = assert(genre.name, 'invalid genre') end
                elseif value(item.genre) then out.tags = {item.genre} end
            end
        end
    end
    if previous then
        self.data[groups[kind]][previous.index] = out
    else
        local list = self.data[groups[kind]]
        list[#list + 1] = out
        previous = {index = #list}
        self.keys[kind][#self.keys[kind] + 1] = key
        previous.dynamic = {item = key, is_external = false}
        self.data.dynamics[#self.data.dynamics + 1] = previous.dynamic
    end
    previous.summary = summary == true
    self.entries[identity] = previous
    if not summary then
        local cover = value(item.coverArt)
        if cover then self.data.images[#self.data.images + 1] = {item_id = key, image_type = 'Primary', native_id = M.native(cover), fresh = cover} end
        local dynamic = previous.dynamic
        copy(dynamic, item, 'remote_play_count', 'playCount')
        copy(dynamic, item, 'remote_last_played_at', 'played', date)
        copy(dynamic, item, 'favorite_at', 'starred', date)
        if kind == 'Song' and value(item.albumId) then
            out.album_id = self:add('Album', {id = item.albumId, name = value(item.album) or item.albumId}, true)
        end
        if kind ~= 'Artist' then
            local targets, present = self:artists(item, 'artists')
            if present then self:relation(kind, key, targets, 'replace') end
            if kind == 'Song' and out.album_id and value(item.albumArtists) then
                local album_artists = self:artists(item, 'albumArtists')
                self:relation('Album', out.album_id, album_artists, 'merge')
            end
        end
    end
    return key
end

return M
