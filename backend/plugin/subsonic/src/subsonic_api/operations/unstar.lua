local R = require("subsonic_api.runtime")
local V = require("subsonic_api.models")

---@class (exact) subsonic_api.unstarParams
---@field id? subsonic_api.models.Schema75
---@field album_id? subsonic_api.models.Schema76
---@field artist_id? subsonic_api.models.Schema77

---@class subsonic_api.unstarOperation
---@field private options subsonic_api.Options
---@field private params subsonic_api.unstarParams
local unstar = {}
unstar.__index = unstar

---@param options subsonic_api.Options
---@param parameters subsonic_api.unstarParams
---@return subsonic_api.unstarOperation
function unstar.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, unstar)
end

---@param request_context? subsonic_api.RequestContext
---@return MluaHttpRequest
function unstar:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/rest/unstar", {}, {}
    do
        local value = params["id"]
        if value ~= nil then
            R.validate(V.check_Schema75, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "id", R.scalar(element) }
            end
        end
    end
    do
        local value = params["album_id"]
        if value ~= nil then
            R.validate(V.check_Schema76, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "albumId", R.scalar(element) }
            end
        end
    end
    do
        local value = params["artist_id"]
        if value ~= nil then
            R.validate(V.check_Schema77, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "artistId", R.scalar(element) }
            end
        end
    end
    R.parameters(params, {["id"] = true, ["album_id"] = true, ["artist_id"] = true, })
    R.authorize(options, {{["apiKeyAuth"] = {}, ["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["legacyPassword"] = {}, ["protocolVersion"] = {}, ["username"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, ["salt"] = {}, ["token"] = {}, ["username"] = {}, }, }, "unstar", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return subsonic_api.models.SubsonicResponse
function unstar:parse_response(response)
    local status = response:status()
    local context = { operation_id = "unstar", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "application/json" then
            return R.json_response(self.options.json, response, context, V.check_SubsonicResponse)
        end
        if R.media_matches(context.content_type, "application/json") then
            return R.json_response(self.options.json, response, context, V.check_SubsonicResponse)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return unstar
