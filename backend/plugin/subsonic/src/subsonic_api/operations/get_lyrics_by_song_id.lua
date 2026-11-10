local R = require("subsonic_api.runtime")
local V = require("subsonic_api.models")

---@class (exact) subsonic_api.get_lyrics_by_song_idParams
---@field id subsonic_api.models.Schema27
---@field enhanced? subsonic_api.models.Schema28

---@class subsonic_api.get_lyrics_by_song_idOperation
---@field private options subsonic_api.Options
---@field private params subsonic_api.get_lyrics_by_song_idParams
local get_lyrics_by_song_id = {}
get_lyrics_by_song_id.__index = get_lyrics_by_song_id

---@param options subsonic_api.Options
---@param parameters subsonic_api.get_lyrics_by_song_idParams
---@return subsonic_api.get_lyrics_by_song_idOperation
function get_lyrics_by_song_id.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, get_lyrics_by_song_id)
end

---@param request_context? subsonic_api.RequestContext
---@return MluaHttpRequest
function get_lyrics_by_song_id:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/rest/getLyricsBySongId", {}, {}
    do
        local value = params["id"]
        if value ~= nil then
            R.validate(V.check_Schema27, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "id", R.scalar(value) }
        else
            R.parameter(false, "missing id")
        end
    end
    do
        local value = params["enhanced"]
        if value ~= nil then
            R.validate(V.check_Schema28, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "enhanced", R.scalar(value) }
        end
    end
    R.parameters(params, {["id"] = true, ["enhanced"] = true, })
    R.authorize(options, {{["apiKeyAuth"] = {}, ["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["legacyPassword"] = {}, ["protocolVersion"] = {}, ["username"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, ["salt"] = {}, ["token"] = {}, ["username"] = {}, }, }, "getLyricsBySongId", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return subsonic_api.models.GetLyricsBySongIdResponse
function get_lyrics_by_song_id:parse_response(response)
    local status = response:status()
    local context = { operation_id = "getLyricsBySongId", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "application/json" then
            return R.json_response(self.options.json, response, context, V.check_GetLyricsBySongIdResponse)
        end
        if R.media_matches(context.content_type, "application/json") then
            return R.json_response(self.options.json, response, context, V.check_GetLyricsBySongIdResponse)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return get_lyrics_by_song_id
