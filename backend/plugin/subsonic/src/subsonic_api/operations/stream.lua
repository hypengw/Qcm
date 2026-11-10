local R = require("subsonic_api.runtime")
local V = require("subsonic_api.models")

---@class (exact) subsonic_api.streamParams
---@field id subsonic_api.models.Schema61
---@field max_bit_rate? subsonic_api.models.Schema62
---@field format? subsonic_api.models.Schema63
---@field time_offset? subsonic_api.models.Schema64
---@field estimate_content_length? subsonic_api.models.Schema65
---@field converted? subsonic_api.models.Schema66

---@class subsonic_api.streamOperation
---@field private options subsonic_api.Options
---@field private params subsonic_api.streamParams
local stream = {}
stream.__index = stream

---@param options subsonic_api.Options
---@param parameters subsonic_api.streamParams
---@return subsonic_api.streamOperation
function stream.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, stream)
end

---@param request_context? subsonic_api.RequestContext
---@return MluaHttpRequest
function stream:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/rest/stream", {}, {}
    do
        local value = params["id"]
        if value ~= nil then
            R.validate(V.check_Schema61, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "id", R.scalar(value) }
        else
            R.parameter(false, "missing id")
        end
    end
    do
        local value = params["max_bit_rate"]
        if value ~= nil then
            R.validate(V.check_Schema62, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "maxBitRate", R.scalar(value) }
        end
    end
    do
        local value = params["format"]
        if value ~= nil then
            R.validate(V.check_Schema63, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "format", R.scalar(value) }
        end
    end
    do
        local value = params["time_offset"]
        if value ~= nil then
            R.validate(V.check_Schema64, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "timeOffset", R.scalar(value) }
        end
    end
    do
        local value = params["estimate_content_length"]
        if value ~= nil then
            R.validate(V.check_Schema65, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "estimateContentLength", R.scalar(value) }
        end
    end
    do
        local value = params["converted"]
        if value ~= nil then
            R.validate(V.check_Schema66, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "converted", R.scalar(value) }
        end
    end
    R.parameters(params, {["id"] = true, ["max_bit_rate"] = true, ["format"] = true, ["time_offset"] = true, ["estimate_content_length"] = true, ["converted"] = true, })
    R.authorize(options, {{["apiKeyAuth"] = {}, ["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["legacyPassword"] = {}, ["protocolVersion"] = {}, ["username"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, ["salt"] = {}, ["token"] = {}, ["username"] = {}, }, }, "stream", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', '*/*' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    builder = builder:stream { headers_timeout_ms = options.headers_timeout_ms or 30000 }
    return builder:build()
end

---@param response MluaHttpResponse
---@return MluaHttpResponse
function stream:parse_response(response)
    local status = response:status()
    local context = { operation_id = "stream", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "application/binary" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema67)
        end
        if context.content_type == "application/json" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_SubsonicResponse)
        end
        if context.content_type == "application/octet-stream" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema68)
        end
        if context.content_type == "audio/*" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema69)
        end
        if context.content_type == "text/xml" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema70)
        end
        if R.media_matches(context.content_type, "application/binary") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema67)
        end
        if R.media_matches(context.content_type, "application/json") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_SubsonicResponse)
        end
        if R.media_matches(context.content_type, "application/octet-stream") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema68)
        end
        if R.media_matches(context.content_type, "text/xml") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema70)
        end
        if R.media_matches(context.content_type, "audio/*") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema69)
        end
        return R.media_error(response, context)
    end
    if status == 206 then
        if context.content_type == "application/binary" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema71)
        end
        if context.content_type == "application/json" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_SubsonicResponse)
        end
        if context.content_type == "application/octet-stream" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema72)
        end
        if context.content_type == "audio/*" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema73)
        end
        if context.content_type == "text/xml" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema74)
        end
        if R.media_matches(context.content_type, "application/binary") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema71)
        end
        if R.media_matches(context.content_type, "application/json") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_SubsonicResponse)
        end
        if R.media_matches(context.content_type, "application/octet-stream") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema72)
        end
        if R.media_matches(context.content_type, "text/xml") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema74)
        end
        if R.media_matches(context.content_type, "audio/*") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema73)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return stream
