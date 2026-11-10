local R = require("subsonic_api.runtime")
local V = require("subsonic_api.models")

---@class (exact) subsonic_api.search3Params
---@field query subsonic_api.models.Schema48
---@field artist_count? subsonic_api.models.Schema49
---@field artist_offset? subsonic_api.models.Schema50
---@field album_count? subsonic_api.models.Schema51
---@field album_offset? subsonic_api.models.Schema52
---@field song_count? subsonic_api.models.Schema53
---@field song_offset? subsonic_api.models.Schema54
---@field music_folder_id? subsonic_api.models.Schema55

---@class subsonic_api.search3Operation
---@field private options subsonic_api.Options
---@field private params subsonic_api.search3Params
local search3 = {}
search3.__index = search3

---@param options subsonic_api.Options
---@param parameters subsonic_api.search3Params
---@return subsonic_api.search3Operation
function search3.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, search3)
end

---@param request_context? subsonic_api.RequestContext
---@return MluaHttpRequest
function search3:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/rest/search3", {}, {}
    do
        local value = params["query"]
        if value ~= nil then
            R.validate(V.check_Schema48, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "query", R.scalar(value) }
        else
            R.parameter(false, "missing query")
        end
    end
    do
        local value = params["artist_count"]
        if value ~= nil then
            R.validate(V.check_Schema49, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "artistCount", R.scalar(value) }
        end
    end
    do
        local value = params["artist_offset"]
        if value ~= nil then
            R.validate(V.check_Schema50, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "artistOffset", R.scalar(value) }
        end
    end
    do
        local value = params["album_count"]
        if value ~= nil then
            R.validate(V.check_Schema51, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "albumCount", R.scalar(value) }
        end
    end
    do
        local value = params["album_offset"]
        if value ~= nil then
            R.validate(V.check_Schema52, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "albumOffset", R.scalar(value) }
        end
    end
    do
        local value = params["song_count"]
        if value ~= nil then
            R.validate(V.check_Schema53, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "songCount", R.scalar(value) }
        end
    end
    do
        local value = params["song_offset"]
        if value ~= nil then
            R.validate(V.check_Schema54, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "songOffset", R.scalar(value) }
        end
    end
    do
        local value = params["music_folder_id"]
        if value ~= nil then
            R.validate(V.check_Schema55, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "musicFolderId", R.scalar(value) }
        end
    end
    R.parameters(params, {["query"] = true, ["artist_count"] = true, ["artist_offset"] = true, ["album_count"] = true, ["album_offset"] = true, ["song_count"] = true, ["song_offset"] = true, ["music_folder_id"] = true, })
    R.authorize(options, {{["apiKeyAuth"] = {}, ["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["legacyPassword"] = {}, ["protocolVersion"] = {}, ["username"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, ["salt"] = {}, ["token"] = {}, ["username"] = {}, }, }, "search3", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return subsonic_api.models.Search3Response
function search3:parse_response(response)
    local status = response:status()
    local context = { operation_id = "search3", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "application/json" then
            return R.json_response(self.options.json, response, context, V.check_Search3Response)
        end
        if R.media_matches(context.content_type, "application/json") then
            return R.json_response(self.options.json, response, context, V.check_Search3Response)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return search3
