package.path = arg[1] .. '/src/?.lua;' .. package.path
local J = runtime.json.strict
local Client = require('jellyfin_api.client')
local count = 0
local transport = { requests = {} }
function transport:request(method, url)
    local record = { method = method, url = url }
    local builder = {}
    function builder:json_codec(codec) assert(codec == 'strict'); return self end
    function builder:query_pairs(pairs) record.query = pairs; return self end
    function builder:header_pairs(pairs) record.headers = pairs; return self end
    function builder:json(body) record.body = J.encode(body); return self end
    function builder:stream(options) record.stream = options; return self end
    function builder:build() transport.requests[#transport.requests + 1] = record; return record end
    return builder
end
local api = Client.new {
    json = J, url = runtime.url, transport = transport, base_url = 'http://host/jellyfin',
    json_media_type = 'application/json; profile="PascalCase"',
    authorize = function() return { headers = { { 'Authorization', 'test-secret' } } } end,
}
local function response(status, content_type, body)
    return {
        reads = 0, closed = false,
        status = function() return status end,
        header_values = function() return content_type and { content_type } or {} end,
        json = function(self, options) assert(options.codec == 'strict'); self.reads = self.reads + 1; return J.decode(body) end,
        text = function(self) self.reads = self.reads + 1; return body end,
        close = function(self) self.closed = true end,
    }
end
local function has_pair(pairs, name, value)
    for _, p in ipairs(pairs) do if p[1] == name and p[2] == value then return true end end
end
local expected = {
    get_artists={'GET','/Artists'}, get_album_artists={'GET','/Artists/AlbumArtists'},
    get_audio_stream={'GET','/Audio/a%2Fb%20%3F/stream'},
    get_item_image={'GET','/Items/a%2Fb%20%3F/Images/Primary'},
    get_items={'GET','/Items'},get_item={'GET','/Items/a%2Fb%20%3F'},
    get_virtual_folders={'GET','/Library/VirtualFolders'},get_lyrics={'GET','/Audio/a%2Fb%20%3F/Lyrics'},
    get_playlist_items={'GET','/Playlists/a%2Fb%20%3F/Items'},get_endpoint_info={'GET','/System/Endpoint'},
    authenticate_user_by_name={'POST','/Users/AuthenticateByName'},
    mark_favorite_item={'POST','/UserFavoriteItems/a%2Fb%20%3F'},
    unmark_favorite_item={'DELETE','/UserFavoriteItems/a%2Fb%20%3F'},get_user_views={'GET','/UserViews'},
}
local ids = {
    authenticate_user_by_name='AuthenticateUserByName', get_album_artists='GetAlbumArtists',
    get_artists='GetArtists', get_audio_stream='GetAudioStream', get_endpoint_info='GetEndpointInfo',
    get_item='GetItem', get_item_image='GetItemImage', get_items='GetItems', get_lyrics='GetLyrics',
    get_playlist_items='GetPlaylistItems', get_user_views='GetUserViews', get_virtual_folders='GetVirtualFolders',
    mark_favorite_item='MarkFavoriteItem', unmark_favorite_item='UnmarkFavoriteItem',
}
for name in pairs(expected) do
    local params = {}
    if expected[name][2]:find('a%%2Fb') then params.item_id = 'a/b ?' end
    if name == 'get_playlist_items' then params.playlist_id = params.item_id; params.item_id = nil end
    if name == 'get_item_image' then params.image_type = 'Primary' end
    if name == 'authenticate_user_by_name' then params.body = { Username = 'test', Pw = 'test' } end
    local operation = api.operations[name](params)
    local req = operation:build_request()
    assert(req.method == expected[name][1])
    assert(req.url == "http://host/jellyfin"..expected[name][2])
    if name=="authenticate_user_by_name" then
        local body=J.decode(req.body);assert(body.Username=="test" and body.Pw=="test")
    end
    assert(req.url:sub(1, #'http://host/jellyfin/') == 'http://host/jellyfin/')
    assert(not req.url:find('{', 1, true))
    assert(has_pair(req.headers, 'Authorization', 'test-secret'))
    local rsp = response(401, nil, 'denied')
    local ok, err = pcall(operation.parse_response, operation, rsp)
    assert(not ok and err.status == 401 and err.operation_id == ids[name] and rsp.reads == 1)
    count = count + 1
end
assert(count == 14)
local params = { parent_id = 'abc', recursive = false, start_index = 0, limit = 1, include_item_types = { 'Audio', 'MusicAlbum' } }
local op = api.operations.get_items(params)
params.limit = 99
local req = op:build_request()
assert(has_pair(req.query, 'limit', '1') and has_pair(req.query, 'recursive', 'false') and has_pair(req.query, 'startIndex', '0'))
assert(has_pair(req.query, 'includeItemTypes', 'Audio') and has_pair(req.query, 'includeItemTypes', 'MusicAlbum'))
local rsp = response(200, 'application/json; charset=utf-8', '{"Items":[],"TotalRecordCount":0,"StartIndex":0}')
local page = op:parse_response(rsp)
assert(#page.Items == 0 and rsp.reads == 1 and J.encode(page.Items) == '[]')
local stream = api.operations.get_audio_stream { item_id = 'abc' }
local partial = response(206, 'audio/mpeg', 'bytes')
assert(stream:parse_response(partial) == partial and partial.reads == 0 and not partial.closed)
local bad = response(416, nil, '')
local ok, err = pcall(stream.parse_response, stream, bad)
assert(not ok and err.status == 416)
local wrong = response(200, 'text/html', '<html>')
assert(not pcall(op.parse_response, op, wrong) and wrong.closed and wrong.reads == 0)
local too_long = api.operations.get_audio_stream { item_id = 'abc', container = string.rep('x',41) }
assert(not pcall(too_long.build_request, too_long))
local object = api.operations.get_audio_stream { item_id = 'abc', stream_options = { one = 'a', two = 'b' } }:build_request()
assert(has_pair(object.query, 'one', 'a') and has_pair(object.query, 'two', 'b'))
print('SDK mock: 14 operations, snapshot, query, model, media and stream checks passed')

assert(api.operations.get_items { include_item_types={} }:build_request())
assert(not pcall(function() api.operations.get_items {include_item_types=J.object {}}:build_request() end))
