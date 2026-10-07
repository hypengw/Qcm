local R = require("jellyfin_api.runtime")
local V = require("jellyfin_api.models")

---@class (exact) jellyfin_api.get_lyricsParams
---@field item_id jellyfin_api.models.Schema65

---@class jellyfin_api.get_lyricsOperation
---@field private options jellyfin_api.Options
---@field private params jellyfin_api.get_lyricsParams
local get_lyrics = {}
get_lyrics.__index = get_lyrics

---@param options jellyfin_api.Options
---@param parameters jellyfin_api.get_lyricsParams
---@return jellyfin_api.get_lyricsOperation
function get_lyrics.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, get_lyrics)
end

---@param request_context? jellyfin_api.RequestContext
---@return MluaHttpRequest
function get_lyrics:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/Audio/{itemId}/Lyrics", {}, {}
    do
        local value = params["item_id"]
        if value ~= nil then
            R.validate(V.check_Schema65, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            path = R.path_parameter(options.url, path, "itemId", value)
        else
            R.parameter(false, "missing item_id")
        end
    end
    R.parameters(params, {["item_id"] = true, })
    R.authorize(options, {{["CustomAuthentication"] = {}, }, }, "GetLyrics", query, headers)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return jellyfin_api.models.LyricDto|jellyfin_api.models.LyricDto|jellyfin_api.models.LyricDto
function get_lyrics:parse_response(response)
    local status = response:status()
    local context = { operation_id = "GetLyrics", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "application/json" then
            return R.json_response(self.options.json, response, context, V.check_LyricDto)
        end
        if context.content_type == "application/json; profile=\"CamelCase\"" then
            return R.json_response(self.options.json, response, context, V.check_LyricDto)
        end
        if context.content_type == "application/json; profile=\"PascalCase\"" then
            return R.json_response(self.options.json, response, context, V.check_LyricDto)
        end
        if R.media_matches(context.content_type, "application/json") then
            return R.json_response(self.options.json, response, context, V.check_LyricDto)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return get_lyrics
