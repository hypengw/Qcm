local M = {}
local J = qcm.json.strict
function M.value(value) if value ~= J.null then return value end end
local value = M.value
function M.native(id)
    assert(type(id) == 'string', 'missing Jellyfin item ID')
    local hex = id:gsub('-', ''):lower()
    assert(#hex == 32 and not hex:find('[^0-9a-f]'), 'invalid Jellyfin item ID')
    return hex:sub(1,8)..'-'..hex:sub(9,12)..'-'..hex:sub(13,16)..'-'..hex:sub(17,20)..'-'..hex:sub(21)
end
function M.key(library, kind, id)
    return { library_id = library or J.null, type = kind, native_id = M.native(id) }
end
local function copy(out, source, target, wire, convert)
    local v = source[wire]
    if v ~= nil then out[target] = v == J.null and J.null or convert and convert(v) or v end
end
local function date(v) return qcm.time.parse_rfc3339(v) end
local function duration(v) assert(math.type(v)=='integer' and v>=0, 'invalid duration'); return v // 10000 end
local function add_images(batch, key, item)
    local tags = value(item.ImageTags)
    if tags then
        for _, kind in ipairs { 'Primary', 'Backdrop', 'Banner', 'Thumb', 'Logo' } do
            if value(tags[kind]) then batch.images[#batch.images+1] = {item_id=key,image_type=kind,native_id=key.native_id,fresh=tags[kind]} end
        end
    end
    if key.type == 'Song' and (not tags or not value(tags.Primary)) and value(item.AlbumId) and value(item.AlbumPrimaryImageTag) then
        batch.images[#batch.images+1] = {item_id=key,image_type='Primary',native_id=M.native(item.AlbumId),fresh=item.AlbumPrimaryImageTag}
    end
end
function M.batch() return { albums={}, artists={}, songs={}, remote_mixes={}, dynamics={}, images={}, album_artists={}, song_artists={} } end
function M.add(batch, library, kind, item)
    local key = M.key(library,kind,item.Id)
    local out = { item=key, name=value(item.Name) or key.native_id }
    if kind ~= 'Mix' then copy(out,item,'sort_name','SortName') end
    if kind == 'Album' or kind == 'Song' then
        if value(item.RunTimeTicks) then out.duration=duration(item.RunTimeTicks) end
        copy(out,item,'publish_time','PremiereDate',date)
        copy(out,item,'added_at','DateCreated',date)
    end
    if kind == 'Artist' then
        if value(item.Overview) then out.description=item.Overview end
        if value(item.AlbumCount) then out.album_count=item.AlbumCount end
        if value(item.SongCount) then out.music_count=item.SongCount end
        batch.artists[#batch.artists+1]=out
    elseif kind == 'Album' then
        copy(out,item,'description','Overview')
        if value(item.ChildCount) then out.track_count=item.ChildCount end
        out.on_insert={disc_count=1}
        batch.albums[#batch.albums+1]=out
    elseif kind == 'Song' then
        if value(item.IndexNumber) then out.track_number=item.IndexNumber end
        if value(item.ParentIndexNumber) then out.disc_number=item.ParentIndexNumber end
        if item.AlbumId ~= nil then out.album_id=value(item.AlbumId) and M.key(library,'Album',item.AlbumId) or J.null end
        if value(item.Tags) then out.tags=item.Tags end
        batch.songs[#batch.songs+1]=out
    elseif kind == 'Mix' then
        copy(out,item,'description','Overview')
        if value(item.ChildCount) then out.track_count=item.ChildCount end
        out.mix_type='user'
        batch.remote_mixes[#batch.remote_mixes+1]=out
    end
    local dynamic={id=key,is_external=false}
    local user=value(item.UserData)
    if user then
        if value(user.IsFavorite) ~= nil then dynamic.favorite=user.IsFavorite end
        if value(user.PlayCount) ~= nil then dynamic.remote_play_count=user.PlayCount end
        copy(dynamic,user,'remote_last_played_at','LastPlayedDate',date)
    end
    batch.dynamics[#batch.dynamics+1]=dynamic
    add_images(batch,key,item)
    local artists = kind == 'Album' and value(item.AlbumArtists) or kind == 'Song' and value(item.ArtistItems)
    if artists then
        local targets, seen={},{}
        for _, artist in ipairs(artists) do
            local target=M.key(library,'Artist',artist.Id)
            if not seen[target.native_id] then targets[#targets+1]=target; seen[target.native_id]=true end
        end
        local group=kind=='Album' and batch.album_artists or batch.song_artists
        group[#group+1]={owner=key,mode='replace',targets=targets}
    end
    return key
end
return M
