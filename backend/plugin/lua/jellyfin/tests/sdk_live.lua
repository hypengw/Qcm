package.path = arg[1] .. '/src/?.lua;' .. package.path
local J = runtime.json.strict
local Client = require('jellyfin_api.client')
local token
local api = Client.new {
    json = J, url = runtime.url, transport = runtime.http,
    base_url = assert(os.getenv('JELLYFIN_URL')),
    json_media_type = 'application/json; profile="PascalCase"',
    authorize = function()
        local header = 'MediaBrowser Client="Qcm SDK test", Device="Linux", DeviceId="qcm-sdk-migration-test", Version="0.1"'
        if token then header = header .. ', Token="' .. token .. '"' end
        return { headers = { { 'Authorization', header } } }
    end,
}
local auth = api:authenticate_user_by_name {
    body = { Username = assert(os.getenv('JELLYFIN_USER')), Pw = assert(os.getenv('JELLYFIN_PASSWORD')) },
}
token = assert(auth.AccessToken)
assert(auth.User.Id)
local views = api:get_user_views { user_id = auth.User.Id }
local music, music_id = 0, nil
for _, item in ipairs(views.Items) do if item.CollectionType == 'music' then music = music + 1; music_id = item.Id end end
local request = api.operations.get_user_views { user_id = auth.User.Id }
local batch = runtime.http:new_batch { results = 'response', limit = 2 }
batch:add('views', request:build_request())
batch:seal()
local result = batch:wait_one()
assert(result.key == 'views' and result.response)
local again = request:parse_response(result.response)
assert(again.TotalRecordCount == views.TotalRecordCount)
assert(batch:wait_one() == nil)
batch:close()
print('SDK live: login, ' .. #views.Items .. ' views, ' .. music .. ' music views; batch parser passed')

if music_id then
    for _, kind in ipairs { 'MusicAlbum', 'Audio', 'Playlist' } do
        local page = api:get_items { user_id = auth.User.Id, parent_id = music_id, include_item_types = { kind }, recursive = true, limit = 2 }
        print('SDK live: ' .. kind .. ', total=' .. page.TotalRecordCount .. ', sampled=' .. #page.Items)
    end
    local artists = api:get_artists { user_id = auth.User.Id, parent_id = music_id, limit = 2 }
    local album_artists = api:get_album_artists { user_id = auth.User.Id, parent_id = music_id, limit = 2 }
    print('SDK live: artists=' .. artists.TotalRecordCount .. ', album artists=' .. album_artists.TotalRecordCount)
end
local lists = api:get_items { user_id = auth.User.Id, include_item_types = {'Playlist'}, recursive = true, limit = 80, start_index = 0 }
local ids = {}
for _, item in ipairs(lists.Items) do ids[item.Id] = (ids[item.Id] or 0) + 1 end
local unique, duplicate = 0, 0
for _, count in pairs(ids) do unique = unique + 1; duplicate = duplicate + count - 1 end
print('SDK live: global playlists total='..lists.TotalRecordCount..', rows='..#lists.Items..', unique='..unique..', duplicates='..duplicate)
for _, sort in ipairs {'Default','SortName'} do
    local ids,seen,count={}, {},0
    for offset=0,9 do
        local page=api:get_items {user_id=auth.User.Id, include_item_types={'Playlist'},recursive=true,limit=1,start_index=offset,sort_by={sort}}
        local types={}
        for _,item in ipairs(page.Items) do count=count+1; if seen[item.Id] then types[#types+1]='duplicate' end; seen[item.Id]=true; types[#types+1]=item.Type end
        print('playlist audit '..sort..' offset='..offset..' total='..page.TotalRecordCount..' returned='..#page.Items..' types='..table.concat(types,','))
    end
end
if music_id then
    local songs = api:get_items { user_id = auth.User.Id, parent_id = music_id, include_item_types = {'Audio'}, recursive = true, limit = 1 }
    local song = assert(songs.Items[1], 'test library requires an audio item')
    local operation = api.operations.get_audio_stream { item_id = song.Id, static = true }
    local prepared = operation:build_request { headers = { {'Range', 'bytes=0-511'} } }
    local response = operation:parse_response(runtime.http:send(prepared))
    assert(response:status() == 206 and response:header_values('content-range')[1])
    assert(#response:bytes { max_bytes = 1024, timeout_ms = 5000 } == 512)
    assert(response:status() == 206)
    local albums = api:get_items { user_id = auth.User.Id, parent_id = music_id, include_item_types = {'MusicAlbum'}, recursive = true, limit = 1 }
    local image = api:get_item_image { item_id = assert(albums.Items[1]).Id, image_type = 'Primary', max_width = 64 }
    assert(image:status() == 200 and #image:bytes { max_bytes = 1024 * 1024, timeout_ms = 5000 } > 0)
    print('SDK live: installed audio Range and image responses passed')
end
