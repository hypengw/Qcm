local R = require("subsonic_api.runtime")
local V = require("subsonic_api.models")

---@class (exact) subsonic_api.get_open_subsonic_extensionsParams

---@class subsonic_api.get_open_subsonic_extensionsOperation
---@field private options subsonic_api.Options
---@field private params subsonic_api.get_open_subsonic_extensionsParams
local get_open_subsonic_extensions = {}
get_open_subsonic_extensions.__index = get_open_subsonic_extensions

---@param options subsonic_api.Options
---@param parameters subsonic_api.get_open_subsonic_extensionsParams
---@return subsonic_api.get_open_subsonic_extensionsOperation
function get_open_subsonic_extensions.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, get_open_subsonic_extensions)
end

---@param request_context? subsonic_api.RequestContext
---@return MluaHttpRequest
function get_open_subsonic_extensions:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/rest/getOpenSubsonicExtensions", {}, {}
    R.parameters(params, {})
    R.authorize(options, {}, "getOpenSubsonicExtensions", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return subsonic_api.models.GetOpenSubsonicExtensionsResponse
function get_open_subsonic_extensions:parse_response(response)
    local status = response:status()
    local context = { operation_id = "getOpenSubsonicExtensions", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "application/json" then
            return R.json_response(self.options.json, response, context, V.check_GetOpenSubsonicExtensionsResponse)
        end
        if R.media_matches(context.content_type, "application/json") then
            return R.json_response(self.options.json, response, context, V.check_GetOpenSubsonicExtensionsResponse)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return get_open_subsonic_extensions
