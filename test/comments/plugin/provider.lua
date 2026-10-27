local qcm = qcm
local p = {}
local reactions = {}
function p.load() end
function p.save() return '' end
function p.check() end
function p.login() return {type='Ok'} end
function p.sync(ctx)
    local lib = ctx:sync_libraries{{library_id=-1,name='Comments',provider_id=qcm.inner.id,native_id='test'}}[1]
    local ids = ctx:allocate_items{{id=-1,native_id='song',provider_id=qcm.inner.id,library_id=lib,type='Song'}}
    ctx:sync_songs{{id=ids[1],name='Comment test song',duration=42000,track_number=1,disc_number=1,can_play=true,popularity=0,tags={}}}
end
function p.sync_item() end
function p.favorite() error('comments must not use media favorites') end
function p.image() error('no images') end
function p.audio() error('no playback') end
function p.subtitle() end
local function comment(id, order)
    return {id=id,author={id='42',name='Listener with a long display name'},
        content=(order == 'Hot' and 'Hot: ' or '') .. 'Comment ' .. id .. '\nUnicode: \228\184\173\230\150\135\240\159\142\181\n<plain text> https://example.invalid/' .. string.rep('long',30),
        time_ms=1793000000000,liked=reactions[id] == true,like_count=reactions[id] and 1 or 0,
        reply_count=1,quote={id='deleted',author={},content='',deleted=true}}
end
function p.comments(ctx, target, request)
    assert(target.native_id == 'song')
    local rows = request.state and {comment('2',request.order),comment('3',request.order)} or {comment('1',request.order),comment('2',request.order)}
    return {comments=rows,order=request.order,orders={'Hot','Latest'},total=3,has_more=not request.state,
        next_state=not request.state and 'next' or nil,can_reply=true,can_like=true}
end
function p.comment_replies(ctx, target, parent, request)
    return {comments={comment('1',request.order)},order=request.order,orders={'Latest'},total=1,has_more=false,can_reply=false,can_like=true}
end
function p.set_comment_like(ctx, target, id, value)
    if id == '2' then qcm.error('deliberate write failure') end
    reactions[id] = value
    return {liked=value}
end
return p
