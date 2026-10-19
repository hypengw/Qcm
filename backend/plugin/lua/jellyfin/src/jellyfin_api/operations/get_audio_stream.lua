local R = require("jellyfin_api.runtime")
local V = require("jellyfin_api.models")

---@class (exact) jellyfin_api.get_audio_streamParams
---@field item_id jellyfin_api.models.Schema66
---@field container? jellyfin_api.models.Schema67
---@field static? jellyfin_api.models.Schema68
---@field params? jellyfin_api.models.Schema69
---@field tag? jellyfin_api.models.Schema70
---@field device_profile_id? jellyfin_api.models.Schema71
---@field play_session_id? jellyfin_api.models.Schema72
---@field segment_container? jellyfin_api.models.Schema73
---@field segment_length? jellyfin_api.models.Schema74
---@field min_segments? jellyfin_api.models.Schema75
---@field media_source_id? jellyfin_api.models.Schema76
---@field device_id? jellyfin_api.models.Schema77
---@field audio_codec? jellyfin_api.models.Schema78
---@field enable_auto_stream_copy? jellyfin_api.models.Schema79
---@field allow_video_stream_copy? jellyfin_api.models.Schema80
---@field allow_audio_stream_copy? jellyfin_api.models.Schema81
---@field break_on_non_key_frames? jellyfin_api.models.Schema82
---@field audio_sample_rate? jellyfin_api.models.Schema83
---@field max_audio_bit_depth? jellyfin_api.models.Schema84
---@field audio_bit_rate? jellyfin_api.models.Schema85
---@field audio_channels? jellyfin_api.models.Schema86
---@field max_audio_channels? jellyfin_api.models.Schema87
---@field profile? jellyfin_api.models.Schema88
---@field level? jellyfin_api.models.Schema89
---@field framerate? jellyfin_api.models.Schema90
---@field max_framerate? jellyfin_api.models.Schema91
---@field copy_timestamps? jellyfin_api.models.Schema92
---@field start_time_ticks? jellyfin_api.models.Schema93
---@field width? jellyfin_api.models.Schema94
---@field height? jellyfin_api.models.Schema95
---@field video_bit_rate? jellyfin_api.models.Schema96
---@field subtitle_stream_index? jellyfin_api.models.Schema97
---@field subtitle_method? jellyfin_api.models.Schema98
---@field max_ref_frames? jellyfin_api.models.Schema99
---@field max_video_bit_depth? jellyfin_api.models.Schema100
---@field require_avc? jellyfin_api.models.Schema101
---@field de_interlace? jellyfin_api.models.Schema102
---@field require_non_anamorphic? jellyfin_api.models.Schema103
---@field transcoding_max_audio_channels? jellyfin_api.models.Schema104
---@field cpu_core_limit? jellyfin_api.models.Schema105
---@field live_stream_id? jellyfin_api.models.Schema106
---@field enable_mpegts_m2_ts_mode? jellyfin_api.models.Schema107
---@field video_codec? jellyfin_api.models.Schema108
---@field subtitle_codec? jellyfin_api.models.Schema109
---@field transcode_reasons? jellyfin_api.models.Schema110
---@field audio_stream_index? jellyfin_api.models.Schema111
---@field video_stream_index? jellyfin_api.models.Schema112
---@field context? jellyfin_api.models.Schema113
---@field stream_options? jellyfin_api.models.Schema114
---@field enable_audio_vbr_encoding? jellyfin_api.models.Schema115

---@class jellyfin_api.get_audio_streamOperation
---@field private options jellyfin_api.Options
---@field private params jellyfin_api.get_audio_streamParams
local get_audio_stream = {}
get_audio_stream.__index = get_audio_stream

---@param options jellyfin_api.Options
---@param parameters jellyfin_api.get_audio_streamParams
---@return jellyfin_api.get_audio_streamOperation
function get_audio_stream.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, get_audio_stream)
end

---@param request_context? jellyfin_api.RequestContext
---@return MluaHttpRequest
function get_audio_stream:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/Audio/{itemId}/stream", {}, {}
    do
        local value = params["item_id"]
        if value ~= nil then
            R.validate(V.check_Schema66, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            path = R.path_parameter(options.url, path, "itemId", value)
        else
            R.parameter(false, "missing item_id")
        end
    end
    do
        local value = params["container"]
        if value ~= nil then
            R.validate(V.check_Schema67, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "container", R.scalar(value) }
        end
    end
    do
        local value = params["static"]
        if value ~= nil then
            R.validate(V.check_Schema68, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "static", R.scalar(value) }
        end
    end
    do
        local value = params["params"]
        if value ~= nil then
            R.validate(V.check_Schema69, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "params", R.scalar(value) }
        end
    end
    do
        local value = params["tag"]
        if value ~= nil then
            R.validate(V.check_Schema70, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "tag", R.scalar(value) }
        end
    end
    do
        local value = params["device_profile_id"]
        if value ~= nil then
            R.validate(V.check_Schema71, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "deviceProfileId", R.scalar(value) }
        end
    end
    do
        local value = params["play_session_id"]
        if value ~= nil then
            R.validate(V.check_Schema72, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "playSessionId", R.scalar(value) }
        end
    end
    do
        local value = params["segment_container"]
        if value ~= nil then
            R.validate(V.check_Schema73, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "segmentContainer", R.scalar(value) }
        end
    end
    do
        local value = params["segment_length"]
        if value ~= nil then
            R.validate(V.check_Schema74, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "segmentLength", R.scalar(value) }
        end
    end
    do
        local value = params["min_segments"]
        if value ~= nil then
            R.validate(V.check_Schema75, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "minSegments", R.scalar(value) }
        end
    end
    do
        local value = params["media_source_id"]
        if value ~= nil then
            R.validate(V.check_Schema76, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "mediaSourceId", R.scalar(value) }
        end
    end
    do
        local value = params["device_id"]
        if value ~= nil then
            R.validate(V.check_Schema77, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "deviceId", R.scalar(value) }
        end
    end
    do
        local value = params["audio_codec"]
        if value ~= nil then
            R.validate(V.check_Schema78, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "audioCodec", R.scalar(value) }
        end
    end
    do
        local value = params["enable_auto_stream_copy"]
        if value ~= nil then
            R.validate(V.check_Schema79, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "enableAutoStreamCopy", R.scalar(value) }
        end
    end
    do
        local value = params["allow_video_stream_copy"]
        if value ~= nil then
            R.validate(V.check_Schema80, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "allowVideoStreamCopy", R.scalar(value) }
        end
    end
    do
        local value = params["allow_audio_stream_copy"]
        if value ~= nil then
            R.validate(V.check_Schema81, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "allowAudioStreamCopy", R.scalar(value) }
        end
    end
    do
        local value = params["break_on_non_key_frames"]
        if value ~= nil then
            R.validate(V.check_Schema82, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "breakOnNonKeyFrames", R.scalar(value) }
        end
    end
    do
        local value = params["audio_sample_rate"]
        if value ~= nil then
            R.validate(V.check_Schema83, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "audioSampleRate", R.scalar(value) }
        end
    end
    do
        local value = params["max_audio_bit_depth"]
        if value ~= nil then
            R.validate(V.check_Schema84, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "maxAudioBitDepth", R.scalar(value) }
        end
    end
    do
        local value = params["audio_bit_rate"]
        if value ~= nil then
            R.validate(V.check_Schema85, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "audioBitRate", R.scalar(value) }
        end
    end
    do
        local value = params["audio_channels"]
        if value ~= nil then
            R.validate(V.check_Schema86, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "audioChannels", R.scalar(value) }
        end
    end
    do
        local value = params["max_audio_channels"]
        if value ~= nil then
            R.validate(V.check_Schema87, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "maxAudioChannels", R.scalar(value) }
        end
    end
    do
        local value = params["profile"]
        if value ~= nil then
            R.validate(V.check_Schema88, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "profile", R.scalar(value) }
        end
    end
    do
        local value = params["level"]
        if value ~= nil then
            R.validate(V.check_Schema89, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "level", R.scalar(value) }
        end
    end
    do
        local value = params["framerate"]
        if value ~= nil then
            R.validate(V.check_Schema90, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "framerate", R.scalar(value) }
        end
    end
    do
        local value = params["max_framerate"]
        if value ~= nil then
            R.validate(V.check_Schema91, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "maxFramerate", R.scalar(value) }
        end
    end
    do
        local value = params["copy_timestamps"]
        if value ~= nil then
            R.validate(V.check_Schema92, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "copyTimestamps", R.scalar(value) }
        end
    end
    do
        local value = params["start_time_ticks"]
        if value ~= nil then
            R.validate(V.check_Schema93, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "startTimeTicks", R.scalar(value) }
        end
    end
    do
        local value = params["width"]
        if value ~= nil then
            R.validate(V.check_Schema94, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "width", R.scalar(value) }
        end
    end
    do
        local value = params["height"]
        if value ~= nil then
            R.validate(V.check_Schema95, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "height", R.scalar(value) }
        end
    end
    do
        local value = params["video_bit_rate"]
        if value ~= nil then
            R.validate(V.check_Schema96, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "videoBitRate", R.scalar(value) }
        end
    end
    do
        local value = params["subtitle_stream_index"]
        if value ~= nil then
            R.validate(V.check_Schema97, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "subtitleStreamIndex", R.scalar(value) }
        end
    end
    do
        local value = params["subtitle_method"]
        if value ~= nil then
            R.validate(V.check_Schema98, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "subtitleMethod", R.scalar(value) }
        end
    end
    do
        local value = params["max_ref_frames"]
        if value ~= nil then
            R.validate(V.check_Schema99, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "maxRefFrames", R.scalar(value) }
        end
    end
    do
        local value = params["max_video_bit_depth"]
        if value ~= nil then
            R.validate(V.check_Schema100, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "maxVideoBitDepth", R.scalar(value) }
        end
    end
    do
        local value = params["require_avc"]
        if value ~= nil then
            R.validate(V.check_Schema101, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "requireAvc", R.scalar(value) }
        end
    end
    do
        local value = params["de_interlace"]
        if value ~= nil then
            R.validate(V.check_Schema102, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "deInterlace", R.scalar(value) }
        end
    end
    do
        local value = params["require_non_anamorphic"]
        if value ~= nil then
            R.validate(V.check_Schema103, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "requireNonAnamorphic", R.scalar(value) }
        end
    end
    do
        local value = params["transcoding_max_audio_channels"]
        if value ~= nil then
            R.validate(V.check_Schema104, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "transcodingMaxAudioChannels", R.scalar(value) }
        end
    end
    do
        local value = params["cpu_core_limit"]
        if value ~= nil then
            R.validate(V.check_Schema105, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "cpuCoreLimit", R.scalar(value) }
        end
    end
    do
        local value = params["live_stream_id"]
        if value ~= nil then
            R.validate(V.check_Schema106, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "liveStreamId", R.scalar(value) }
        end
    end
    do
        local value = params["enable_mpegts_m2_ts_mode"]
        if value ~= nil then
            R.validate(V.check_Schema107, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "enableMpegtsM2TsMode", R.scalar(value) }
        end
    end
    do
        local value = params["video_codec"]
        if value ~= nil then
            R.validate(V.check_Schema108, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "videoCodec", R.scalar(value) }
        end
    end
    do
        local value = params["subtitle_codec"]
        if value ~= nil then
            R.validate(V.check_Schema109, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "subtitleCodec", R.scalar(value) }
        end
    end
    do
        local value = params["transcode_reasons"]
        if value ~= nil then
            R.validate(V.check_Schema110, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "transcodeReasons", R.scalar(value) }
        end
    end
    do
        local value = params["audio_stream_index"]
        if value ~= nil then
            R.validate(V.check_Schema111, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "audioStreamIndex", R.scalar(value) }
        end
    end
    do
        local value = params["video_stream_index"]
        if value ~= nil then
            R.validate(V.check_Schema112, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "videoStreamIndex", R.scalar(value) }
        end
    end
    do
        local value = params["context"]
        if value ~= nil then
            R.validate(V.check_Schema113, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "context", R.scalar(value) }
        end
    end
    do
        local value = params["stream_options"]
        if value ~= nil then
            R.validate(V.check_Schema114, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, key in ipairs(R.sorted_keys(value)) do
                query[#query + 1] = { key, R.scalar(value[key]) }
            end
        end
    end
    do
        local value = params["enable_audio_vbr_encoding"]
        if value ~= nil then
            R.validate(V.check_Schema115, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "enableAudioVbrEncoding", R.scalar(value) }
        end
    end
    R.parameters(params, {["item_id"] = true, ["container"] = true, ["static"] = true, ["params"] = true, ["tag"] = true, ["device_profile_id"] = true, ["play_session_id"] = true, ["segment_container"] = true, ["segment_length"] = true, ["min_segments"] = true, ["media_source_id"] = true, ["device_id"] = true, ["audio_codec"] = true, ["enable_auto_stream_copy"] = true, ["allow_video_stream_copy"] = true, ["allow_audio_stream_copy"] = true, ["break_on_non_key_frames"] = true, ["audio_sample_rate"] = true, ["max_audio_bit_depth"] = true, ["audio_bit_rate"] = true, ["audio_channels"] = true, ["max_audio_channels"] = true, ["profile"] = true, ["level"] = true, ["framerate"] = true, ["max_framerate"] = true, ["copy_timestamps"] = true, ["start_time_ticks"] = true, ["width"] = true, ["height"] = true, ["video_bit_rate"] = true, ["subtitle_stream_index"] = true, ["subtitle_method"] = true, ["max_ref_frames"] = true, ["max_video_bit_depth"] = true, ["require_avc"] = true, ["de_interlace"] = true, ["require_non_anamorphic"] = true, ["transcoding_max_audio_channels"] = true, ["cpu_core_limit"] = true, ["live_stream_id"] = true, ["enable_mpegts_m2_ts_mode"] = true, ["video_codec"] = true, ["subtitle_codec"] = true, ["transcode_reasons"] = true, ["audio_stream_index"] = true, ["video_stream_index"] = true, ["context"] = true, ["stream_options"] = true, ["enable_audio_vbr_encoding"] = true, })
    R.authorize(options, {}, "GetAudioStream", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', '*/*' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    builder = builder:stream { headers_timeout_ms = options.headers_timeout_ms or 30000 }
    return builder:build()
end

---@param response MluaHttpResponse
---@return MluaHttpResponse
function get_audio_stream:parse_response(response)
    local status = response:status()
    local context = { operation_id = "GetAudioStream", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "audio/*" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema116)
        end
        if R.media_matches(context.content_type, "audio/*") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema116)
        end
        return R.media_error(response, context)
    end
    if status == 206 then
        if context.content_type == "audio/*" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, nil)
        end
        if R.media_matches(context.content_type, "audio/*") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, nil)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return get_audio_stream
