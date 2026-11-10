local Client = require('jellyfin_api.client')
local map = require('mapping')
local J = qcm.json.strict
local M = {}
local token, user_id, api, generation = nil, nil, nil, 0
local load_error
local function raise(value)
    if type(value) == 'table' then qcm.error(value) else error(value,0) end
end
local function quoted(value)
    assert(type(value)=='string' and not value:find('[\r\n]'), 'invalid authentication value')
    return '"' .. value:gsub('\\','\\\\'):gsub('"','\\"') .. '"'
end
local function authorization(saved_generation)
    assert(generation==saved_generation,'account changed')
    local parts={'Client='..quoted(qcm.inner.app_name),'Version='..quoted(qcm.inner.app_version),'Device="Linux"','DeviceId='..quoted(qcm.inner:device_id())}
    if token then parts[#parts+1]='Token='..quoted(token) end
    return {{'Authorization','MediaBrowser '..table.concat(parts,', ')}}
end
local function client(base_url)
    local saved_generation=generation
    return Client.new {
        base_url=base_url, json=J, url=qcm.url, transport=qcm.get_http_client(),
        json_media_type='application/json; profile="PascalCase"',
        authorize=function() return {headers=authorization(saved_generation)} end,
    }
end
local function require_api()
    if load_error then error(load_error,0) end
    if not api or not token then qcm.error {code='not_auth'} end
    return api
end
local function perform(name, params, current)
    current=current or require_api()
    local active=generation
    local ok,result=pcall(current[name],current,params)
    assert(active==generation,'account changed during request')
    if not ok then raise(result) end
    return result
end
function M.load(data)
    generation=generation+1
    token,user_id,api,load_error=nil,nil,nil,nil
    if data=='' then return end
    local ok,state=pcall(J.decode,data)
    if not ok then load_error='invalid saved Jellyfin authentication';error(load_error,0) end
    token,user_id=map.value(state.token),map.value(state.user_id)
    if token and type(token)~='string' then load_error='invalid saved Jellyfin token';error(load_error,0) end
    if token then api=client(qcm.inner.base_url) end
end
function M.save()
    if load_error then error(load_error,0) end
    return J.encode(J.object {token=token or J.null,user_id=user_id or J.null})
end
function M.login(info)
    if not info.method or info.method.type~='Username' then return {type='Failed',message={message='Username authentication is required'}} end
    generation=generation+1
    token,user_id,load_error=nil,nil,nil
    local pending=client(info.server_url)
    local auth=perform('authenticate_user_by_name',{body={Username=info.method.username,Pw=info.method.pw}},pending)
    token=assert(map.value(auth.AccessToken),'missing access token')
    user_id=auth.User and map.value(auth.User.Id)
    api=pending
    return {type='Ok'}
end
function M.check() perform('get_endpoint_info',{}) end

local function pages(name, params, consume)
    local offset,total,seen=0,nil,{}
    while true do
        params.start_index,params.limit=offset,80
        local page=perform(name,params)
        assert(type(page.Items)=='table' and math.type(page.TotalRecordCount)=='integer' and page.TotalRecordCount>=0,'incomplete page metadata')
        if total then assert(total==page.TotalRecordCount,'collection changed during pagination') else total=page.TotalRecordCount end
        assert(total<=200000,'collection exceeds synchronization limit')
        if page.StartIndex~=nil then assert(page.StartIndex==offset,'unexpected page offset') end
        for _,item in ipairs(page.Items) do
            local id=map.native(item.Id)
            assert(not seen[id],'duplicate item in paginated collection')
            seen[id]=true
        end
        assert(offset+#page.Items<=total,'page exceeds reported total')
        consume(page.Items)
        offset=offset+#page.Items
        if offset==total then return end
        assert(#page.Items>0,'pagination stopped before completion')
    end
end
local function scope(library,kind) return {library_id=library or J.null,item_type=qcm.enum.ItemType[kind]} end
local function sync_pages(ctx,run,library,kind,operation,params)
    pages(operation,params,function(items)
        local batch,keys=map.batch(),{}
        local artist_keys, artist_seen={},{}
        for _,item in ipairs(items) do
            keys[#keys+1]=map.add(batch,library,kind,item)
            local artists=kind=='Album' and map.value(item.AlbumArtists) or kind=='Song' and map.value(item.ArtistItems)
            for _,artist in ipairs(artists or {}) do
                local key=map.key(library,'Artist',artist.Id)
                if not artist_seen[key.native_id] then
                    artist_seen[key.native_id]=true
                    artist_keys[#artist_keys+1]=key
                    batch.artists[#batch.artists+1]={item=key,on_insert={name=map.value(artist.Name) or key.native_id}}
                end
            end
        end
        if #keys>0 then
            run:sync_batch(batch)
            run:observe_items(scope(library,kind),keys)
            if #artist_keys>0 then run:observe_items(scope(library,'Artist'),artist_keys) end
        end
    end)
end
function M.sync(ctx)
    local views=perform('get_user_views',{user_id=user_id})
    assert(type(views.Items)=='table' and #views.Items==views.TotalRecordCount,'incomplete library enumeration')
    local inputs,seen={},{}
    for _,view in ipairs(views.Items) do
        if view.CollectionType=='music' then
            assert(type(view.Id)=='string' and not seen[view.Id],'invalid library identity')
            seen[view.Id]=true
            inputs[#inputs+1]={native_id=view.Id,name=map.value(view.Name) or view.Id}
        end
    end
    local libraries=ctx:sync_batch {libraries=inputs}.libraries
    local ids,scopes={}, {scope(nil,'Mix')}
    for _,lib in ipairs(libraries) do
        ids[#ids+1]=lib.library_id
        for _,kind in ipairs {'Artist','Album','Song'} do scopes[#scopes+1]=scope(lib.library_id,kind) end
    end
    local run=ctx:begin_sync {libraries=ids,scopes=scopes}
    run:complete('libraries')
    for _,lib in ipairs(libraries) do
        for _,operation in ipairs {'get_album_artists','get_artists'} do
            sync_pages(ctx,run,lib.library_id,'Artist',operation,{parent_id=lib.native_id,user_id=user_id,fields={'Overview','DateCreated'},enable_user_data=true})
        end
        for _,kind in ipairs {'Album','Song'} do
            sync_pages(ctx,run,lib.library_id,kind,'get_items',{
                parent_id=lib.native_id,user_id=user_id,include_item_types={kind=='Album' and 'MusicAlbum' or 'Audio'},recursive=true,
                fields={'Overview','DateCreated','Tags','ChildCount','SortName'},enable_user_data=true,sort_by={'SortName'},
            })
            run:complete(scope(lib.library_id,kind))
        end
        run:complete(scope(lib.library_id,'Artist'))
        sync_pages(ctx,run,nil,'Mix','get_items',{parent_id=lib.native_id,user_id=user_id,include_item_types={'Playlist'},recursive=true,fields={'Overview','ChildCount'},enable_user_data=true})
    end
    run:complete(scope(nil,'Mix'))
    run:seal()
end
function M.sync_item(ctx,item)
    local remote=perform('get_item',{item_id=item.native_id,user_id=user_id})
    local batch=map.batch()
    local library=map.value(item.library_id)
    map.add(batch,library,item.type,remote)
    if item.type=='Mix' then
        local ids={}
        pages('get_playlist_items',{playlist_id=item.native_id,user_id=user_id},function(items)
            for _,song in ipairs(items) do ids[#ids+1]=map.native(song.Id); assert(#ids<=10000,'playlist exceeds limit') end
        end)
        local songs={}
        for first=1,#ids,500 do
            local chunk={}
            for i=first,math.min(first+499,#ids) do chunk[#chunk+1]=ids[i] end
            for _,song in ipairs(ctx:resolve_items('Song',chunk)) do songs[#songs+1]=song.id end
        end
        batch.remote_mix_contents={{mix=item.id,mode='replace',songs=songs}}
    elseif item.type~='Song' and item.type~='Artist' and item.type~='Album' then qcm.error {code='not_implemented'} end
    ctx:sync_batch(batch)
end
function M.favorite(ctx,id,kind,value)
    perform(value and 'mark_favorite_item' or 'unmark_favorite_item',{item_id=id,user_id=user_id})
end
local function stream(name,params,headers)
    local current=require_api()
    local op=current.operations[name](params)
    local pairs_list={}
    for key,value in pairs(headers or {}) do pairs_list[#pairs_list+1]={key,value} end
    local response=qcm.get_http_client():send(op:build_request {headers=pairs_list})
    local ok,result=pcall(op.parse_response,op,response)
    if not ok then raise(result) end
    return result
end
function M.image(id,image_id,kind)
    return stream('get_item_image',{item_id=image_id or id,image_type=kind,fill_height=420,fill_width=400})
end
function M.prepare_image(id,image_id,kind)
    return require_api().operations.get_item_image {item_id=image_id or id,image_type=kind,fill_height=420,fill_width=400}:build_request()
end
function M.prepare_audio(id)
    return require_api().operations.get_audio_stream {item_id=id,static=true}:build_request()
end
function M.audio(id,headers) return stream('get_audio_stream',{item_id=id,static=true},headers) end
local function playback_state(encoded)
    local state=J.decode(encoded)
    assert(state.generation==generation,'playback account changed')
    require_api()
    return state
end
function M.settings_definition()
    return {
        {key='playback_mode',group='Playback',name='Playback mode',kind='choice',default_value='original',
            description='Applies the next time a track is opened.',options={
                {value='original',name='Original file'},{value='hls',name='HLS transcoding'}}},
        {key='max_bitrate',group='Playback',name='Maximum bitrate',kind='choice',default_value=320000,
            enabled_when={key='playback_mode',value='hls'},options={
                {value=128000,name='128 kbps'},{value=192000,name='192 kbps'},
                {value=256000,name='256 kbps'},{value=320000,name='320 kbps'}}},
    }
end
function M.open_playback(request)
    local settings=qcm.inner:settings()
    if settings.playback_mode=='original' then return {kind='byte_resource'} end
    local bitrate=math.min(map.value(request.max_bitrate) or settings.max_bitrate,settings.max_bitrate)
    assert(math.type(bitrate)=='integer' and bitrate>=32000 and bitrate<=512000,'unsupported audio bitrate')
    local profile={Name='Qcm audio HLS',MaxStreamingBitrate=bitrate,DirectPlayProfiles=J.array{},TranscodingProfiles={{
        Container='ts',Type='Audio',AudioCodec='aac',Protocol='hls',Context='Streaming',MaxAudioChannels='2',
        MinSegments=1,SegmentLength=6,EstimateContentLength=false,TranscodeSeekInfo='Auto',
    }}}
    local result=perform('get_posted_playback_info',{item_id=request.item_id,body={
        UserId=user_id,EnableDirectPlay=false,EnableDirectStream=false,EnableTranscoding=true,
        AllowAudioStreamCopy=false,MaxStreamingBitrate=bitrate,DeviceProfile=profile,
    }})
    local session=map.value(result.PlaySessionId)
    local ok,playback=pcall(function()
        assert(not map.value(result.ErrorCode),'Jellyfin rejected playback negotiation')
        assert(type(session)=='string' and session~='','missing playback session')
        local source
        for _,candidate in ipairs(result.MediaSources or {}) do
            if candidate.SupportsTranscoding and candidate.TranscodingSubProtocol=='hls' and candidate.TranscodingContainer=='ts' then source=candidate;break end
        end
        assert(source and map.value(source.TranscodingUrl),'Jellyfin did not negotiate supported audio HLS')
        local uri=assert(qcm.url.join(qcm.inner.base_url:gsub('/?$','/'),source.TranscodingUrl:gsub('^/','')),'invalid playback URI')
        local root=assert(qcm.url.parse(uri),'invalid playback URI')
        local state=J.encode {generation=generation,session=session,uri=uri,prefix=assert(root.path:match('^(.*)/[^/]+$'))..'/',device=qcm.inner:device_id()}
        return {kind='hls_vod',uri=uri,state=state,representation=J.encode {source.Id,map.value(source.ETag) or '',bitrate,'aac','ts',2,session},duration_seconds=map.value(source.RunTimeTicks) and source.RunTimeTicks/10000000}
    end)
    if not ok then
        if type(session)=='string' and session~='' then pcall(perform,'stop_encoding_process',{device_id=qcm.inner:device_id(),play_session_id=session}) end
        raise(playback)
    end
    return playback
end
function M.prepare_playback_resource(encoded,uri)
    local state=playback_state(encoded)
    local root,target=qcm.url.parse(state.uri),qcm.url.parse(uri)
    assert(target and target.scheme==root.scheme and target.host==root.host and target.port==root.port and target.path:sub(1,#state.prefix)==state.prefix,'resource outside playback scope')
    local extension=target.path:match('%.([%w]+)$')
    assert(extension=='m3u8' or extension=='ts' or extension=='m4s' or extension=='mp4','unsupported playback resource')
    local query=qcm.url.decode(target.query or '')
    assert(not query.PlaySessionId or query.PlaySessionId==state.session,'playback session mismatch')
    local request=qcm.get_http_client():request('GET',uri):header_pairs(authorization(generation)):stream {headers_timeout_ms=10000}:build()
    return qcm.resource.prepare(request,function(response)
        local status=response:status()
        if status==401 or status==403 then qcm.error {code='not_auth'} end
        if status==404 then qcm.error {code='not_found'} end
        if status==304 or status==416 then return end
        assert(status==200 or status==206,'Jellyfin playback HTTP failure')
        local content=(response:header_values('content-type')[1] or ''):lower():match('^[^;]+')
        local allowed=extension=='m3u8' and (content=='application/vnd.apple.mpegurl' or content=='application/x-mpegurl' or content=='audio/mpegurl' or content=='audio/x-mpegurl')
            or extension=='ts' and content=='video/mp2t'
            or (extension=='mp4' or extension=='m4s') and (content=='video/mp4' or content=='audio/mp4' or content=='application/mp4')
        assert(allowed,'invalid Jellyfin playback media type')
    end,{cache_mode=extension=='m3u8' and 'document' or 'whole_object',transient_query={'api_key'}})
end
function M.keep_playback(encoded)
    local state=playback_state(encoded)
    perform('ping_playback_session',{play_session_id=state.session})
end
function M.close_playback(encoded)
    local state=J.decode(encoded)
    if state.generation~=generation then return end
    perform('stop_encoding_process',{device_id=state.device,play_session_id=state.session})
end
function M.subtitle(id)
    local response=perform('get_lyrics',{item_id=id})
    if not map.value(response.Lyrics) then qcm.error {code='not_found'} end
    local items={}
    for _,line in ipairs(response.Lyrics) do
        items[#items+1]={start=map.value(line.Start) and line.Start//10000 or nil,text=map.value(line.Text)}
    end
    return {items=items}
end
return M
