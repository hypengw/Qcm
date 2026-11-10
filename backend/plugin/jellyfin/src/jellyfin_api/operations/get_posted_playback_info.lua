local R = require("jellyfin_api.runtime")
local V = require("jellyfin_api.models")

---@class (exact) jellyfin_api.get_posted_playback_infoParams
---@field item_id jellyfin_api.models.Schema223
---@field user_id? jellyfin_api.models.Schema224
---@field max_streaming_bitrate? jellyfin_api.models.Schema225
---@field start_time_ticks? jellyfin_api.models.Schema226
---@field audio_stream_index? jellyfin_api.models.Schema227
---@field subtitle_stream_index? jellyfin_api.models.Schema228
---@field max_audio_channels? jellyfin_api.models.Schema229
---@field media_source_id? jellyfin_api.models.Schema230
---@field live_stream_id? jellyfin_api.models.Schema231
---@field auto_open_live_stream? jellyfin_api.models.Schema232
---@field enable_direct_play? jellyfin_api.models.Schema233
---@field enable_direct_stream? jellyfin_api.models.Schema234
---@field enable_transcoding? jellyfin_api.models.Schema235
---@field allow_video_stream_copy? jellyfin_api.models.Schema236
---@field allow_audio_stream_copy? jellyfin_api.models.Schema237
---@field body? jellyfin_api.models.Schema239

---@class jellyfin_api.get_posted_playback_infoOperation
---@field private options jellyfin_api.Options
---@field private params jellyfin_api.get_posted_playback_infoParams
local get_posted_playback_info = {}
get_posted_playback_info.__index = get_posted_playback_info

---@param options jellyfin_api.Options
---@param parameters jellyfin_api.get_posted_playback_infoParams
---@return jellyfin_api.get_posted_playback_infoOperation
function get_posted_playback_info.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, get_posted_playback_info)
end

---@param request_context? jellyfin_api.RequestContext
---@return MluaHttpRequest
function get_posted_playback_info:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/Items/{itemId}/PlaybackInfo", {}, {}
    do
        local value = params["item_id"]
        if value ~= nil then
            R.validate(V.check_Schema223, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            path = R.path_parameter(options.url, path, "itemId", value)
        else
            R.parameter(false, "missing item_id")
        end
    end
    do
        local value = params["user_id"]
        if value ~= nil then
            R.validate(V.check_Schema224, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "userId", R.scalar(value) }
        end
    end
    do
        local value = params["max_streaming_bitrate"]
        if value ~= nil then
            R.validate(V.check_Schema225, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "maxStreamingBitrate", R.scalar(value) }
        end
    end
    do
        local value = params["start_time_ticks"]
        if value ~= nil then
            R.validate(V.check_Schema226, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "startTimeTicks", R.scalar(value) }
        end
    end
    do
        local value = params["audio_stream_index"]
        if value ~= nil then
            R.validate(V.check_Schema227, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "audioStreamIndex", R.scalar(value) }
        end
    end
    do
        local value = params["subtitle_stream_index"]
        if value ~= nil then
            R.validate(V.check_Schema228, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "subtitleStreamIndex", R.scalar(value) }
        end
    end
    do
        local value = params["max_audio_channels"]
        if value ~= nil then
            R.validate(V.check_Schema229, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "maxAudioChannels", R.scalar(value) }
        end
    end
    do
        local value = params["media_source_id"]
        if value ~= nil then
            R.validate(V.check_Schema230, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "mediaSourceId", R.scalar(value) }
        end
    end
    do
        local value = params["live_stream_id"]
        if value ~= nil then
            R.validate(V.check_Schema231, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "liveStreamId", R.scalar(value) }
        end
    end
    do
        local value = params["auto_open_live_stream"]
        if value ~= nil then
            R.validate(V.check_Schema232, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "autoOpenLiveStream", R.scalar(value) }
        end
    end
    do
        local value = params["enable_direct_play"]
        if value ~= nil then
            R.validate(V.check_Schema233, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "enableDirectPlay", R.scalar(value) }
        end
    end
    do
        local value = params["enable_direct_stream"]
        if value ~= nil then
            R.validate(V.check_Schema234, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "enableDirectStream", R.scalar(value) }
        end
    end
    do
        local value = params["enable_transcoding"]
        if value ~= nil then
            R.validate(V.check_Schema235, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "enableTranscoding", R.scalar(value) }
        end
    end
    do
        local value = params["allow_video_stream_copy"]
        if value ~= nil then
            R.validate(V.check_Schema236, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "allowVideoStreamCopy", R.scalar(value) }
        end
    end
    do
        local value = params["allow_audio_stream_copy"]
        if value ~= nil then
            R.validate(V.check_Schema237, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "allowAudioStreamCopy", R.scalar(value) }
        end
    end
    R.parameters(params, { body = true, ["item_id"] = true, ["user_id"] = true, ["max_streaming_bitrate"] = true, ["start_time_ticks"] = true, ["audio_stream_index"] = true, ["subtitle_stream_index"] = true, ["max_audio_channels"] = true, ["media_source_id"] = true, ["live_stream_id"] = true, ["auto_open_live_stream"] = true, ["enable_direct_play"] = true, ["enable_direct_stream"] = true, ["enable_transcoding"] = true, ["allow_video_stream_copy"] = true, ["allow_audio_stream_copy"] = true, })
    R.authorize(options, {{["CustomAuthentication"] = {}, }, }, "GetPostedPlaybackInfo", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("POST", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    if params.body ~= nil then
        R.validate(V.check_Schema239, J, params.body)
        builder = builder:json(params.body)
    end
    return builder:build()
end

---@param response MluaHttpResponse
---@return jellyfin_api.models.PlaybackInfoResponse|jellyfin_api.models.PlaybackInfoResponse|jellyfin_api.models.PlaybackInfoResponse
function get_posted_playback_info:parse_response(response)
    local status = response:status()
    local context = { operation_id = "GetPostedPlaybackInfo", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "application/json" then
            return R.json_response(self.options.json, response, context, V.check_PlaybackInfoResponse)
        end
        if context.content_type == "application/json; profile=\"CamelCase\"" then
            return R.json_response(self.options.json, response, context, V.check_PlaybackInfoResponse)
        end
        if context.content_type == "application/json; profile=\"PascalCase\"" then
            return R.json_response(self.options.json, response, context, V.check_PlaybackInfoResponse)
        end
        if R.media_matches(context.content_type, "application/json") then
            return R.json_response(self.options.json, response, context, V.check_PlaybackInfoResponse)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return get_posted_playback_info
