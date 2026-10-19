local R = require("subsonic_api.runtime")
local V = require("subsonic_api.models")

---@class (exact) subsonic_api.get_artistParams
---@field id subsonic_api.models.Schema10

---@class subsonic_api.get_artistOperation
---@field private options subsonic_api.Options
---@field private params subsonic_api.get_artistParams
local get_artist = {}
get_artist.__index = get_artist

---@param options subsonic_api.Options
---@param parameters subsonic_api.get_artistParams
---@return subsonic_api.get_artistOperation
function get_artist.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, get_artist)
end

---@param request_context? subsonic_api.RequestContext
---@return MluaHttpRequest
function get_artist:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/rest/getArtist", {}, {}
    do
        local value = params["id"]
        if value ~= nil then
            R.validate(V.check_Schema10, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "id", R.scalar(value) }
        else
            R.parameter(false, "missing id")
        end
    end
    R.parameters(params, {["id"] = true, })
    R.authorize(options, {{["apiKeyAuth"] = {}, ["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["legacyPassword"] = {}, ["protocolVersion"] = {}, ["username"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, ["salt"] = {}, ["token"] = {}, ["username"] = {}, }, }, "getArtist", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return subsonic_api.models.GetArtistResponse
function get_artist:parse_response(response)
    local status = response:status()
    local context = { operation_id = "getArtist", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "application/json" then
            return R.json_response(self.options.json, response, context, V.check_GetArtistResponse)
        end
        if R.media_matches(context.content_type, "application/json") then
            return R.json_response(self.options.json, response, context, V.check_GetArtistResponse)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return get_artist
