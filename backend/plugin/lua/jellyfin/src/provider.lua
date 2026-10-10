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
local function client(base_url)
    local saved_generation=generation
    return Client.new {
        base_url=base_url, json=J, url=qcm.url, transport=qcm.get_http_client(),
        json_media_type='application/json; profile="PascalCase"',
        authorize=function()
            assert(generation==saved_generation,'account changed')
            local parts={'Client='..quoted(qcm.inner.app_name),'Version='..quoted(qcm.inner.app_version),'Device="Linux"','DeviceId='..quoted(qcm.inner:device_id())}
            if token then parts[#parts+1]='Token='..quoted(token) end
            return {headers={{'Authorization','MediaBrowser '..table.concat(parts,', ')}}}
        end,
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
