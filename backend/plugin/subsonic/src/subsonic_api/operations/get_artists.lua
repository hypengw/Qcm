local R = require("subsonic_api.runtime")
local V = require("subsonic_api.models")

---@class (exact) subsonic_api.get_artistsParams
---@field music_folder_id? subsonic_api.models.Schema12

---@class subsonic_api.get_artistsOperation
---@field private options subsonic_api.Options
---@field private params subsonic_api.get_artistsParams
local get_artists = {}
get_artists.__index = get_artists

---@param options subsonic_api.Options
---@param parameters subsonic_api.get_artistsParams
---@return subsonic_api.get_artistsOperation
function get_artists.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, get_artists)
end

---@param request_context? subsonic_api.RequestContext
---@return MluaHttpRequest
function get_artists:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/rest/getArtists", {}, {}
    do
        local value = params["music_folder_id"]
        if value ~= nil then
            R.validate(V.check_Schema12, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "musicFolderId", R.scalar(value) }
        end
    end
    R.parameters(params, {["music_folder_id"] = true, })
    R.authorize(options, {{["apiKeyAuth"] = {}, ["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["legacyPassword"] = {}, ["protocolVersion"] = {}, ["username"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, ["salt"] = {}, ["token"] = {}, ["username"] = {}, }, }, "getArtists", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return subsonic_api.models.GetArtistsResponse
function get_artists:parse_response(response)
    local status = response:status()
    local context = { operation_id = "getArtists", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "application/json" then
            return R.json_response(self.options.json, response, context, V.check_GetArtistsResponse)
        end
        if R.media_matches(context.content_type, "application/json") then
            return R.json_response(self.options.json, response, context, V.check_GetArtistsResponse)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return get_artists
