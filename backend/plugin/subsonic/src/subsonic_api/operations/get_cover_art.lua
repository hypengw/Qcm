local R = require("subsonic_api.runtime")
local V = require("subsonic_api.models")

---@class (exact) subsonic_api.get_cover_artParams
---@field id subsonic_api.models.Schema14
---@field size? subsonic_api.models.Schema15

---@class subsonic_api.get_cover_artOperation
---@field private options subsonic_api.Options
---@field private params subsonic_api.get_cover_artParams
local get_cover_art = {}
get_cover_art.__index = get_cover_art

---@param options subsonic_api.Options
---@param parameters subsonic_api.get_cover_artParams
---@return subsonic_api.get_cover_artOperation
function get_cover_art.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, get_cover_art)
end

---@param request_context? subsonic_api.RequestContext
---@return MluaHttpRequest
function get_cover_art:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/rest/getCoverArt", {}, {}
    do
        local value = params["id"]
        if value ~= nil then
            R.validate(V.check_Schema14, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "id", R.scalar(value) }
        else
            R.parameter(false, "missing id")
        end
    end
    do
        local value = params["size"]
        if value ~= nil then
            R.validate(V.check_Schema15, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "size", R.scalar(value) }
        end
    end
    R.parameters(params, {["id"] = true, ["size"] = true, })
    R.authorize(options, {{["apiKeyAuth"] = {}, ["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["legacyPassword"] = {}, ["protocolVersion"] = {}, ["username"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, ["salt"] = {}, ["token"] = {}, ["username"] = {}, }, }, "getCoverArt", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', '*/*' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    builder = builder:stream { headers_timeout_ms = options.headers_timeout_ms or 30000 }
    return builder:build()
end

---@param response MluaHttpResponse
---@return MluaHttpResponse
function get_cover_art:parse_response(response)
    local status = response:status()
    local context = { operation_id = "getCoverArt", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "application/binary" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema16)
        end
        if context.content_type == "application/json" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_SubsonicResponse)
        end
        if context.content_type == "application/octet-stream" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema17)
        end
        if context.content_type == "image/*" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema18)
        end
        if context.content_type == "text/xml" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema19)
        end
        if R.media_matches(context.content_type, "application/binary") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema16)
        end
        if R.media_matches(context.content_type, "application/json") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_SubsonicResponse)
        end
        if R.media_matches(context.content_type, "application/octet-stream") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema17)
        end
        if R.media_matches(context.content_type, "text/xml") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema19)
        end
        if R.media_matches(context.content_type, "image/*") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema18)
        end
        return R.media_error(response, context)
    end
    if status == 206 then
        if context.content_type == "application/binary" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema20)
        end
        if context.content_type == "application/json" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_SubsonicResponse)
        end
        if context.content_type == "application/octet-stream" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema21)
        end
        if context.content_type == "image/*" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema22)
        end
        if context.content_type == "text/xml" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema23)
        end
        if R.media_matches(context.content_type, "application/binary") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema20)
        end
        if R.media_matches(context.content_type, "application/json") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_SubsonicResponse)
        end
        if R.media_matches(context.content_type, "application/octet-stream") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema21)
        end
        if R.media_matches(context.content_type, "text/xml") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema23)
        end
        if R.media_matches(context.content_type, "image/*") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema22)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return get_cover_art
