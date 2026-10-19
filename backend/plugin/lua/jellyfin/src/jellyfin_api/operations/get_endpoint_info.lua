local R = require("jellyfin_api.runtime")
local V = require("jellyfin_api.models")

---@class (exact) jellyfin_api.get_endpoint_infoParams

---@class jellyfin_api.get_endpoint_infoOperation
---@field private options jellyfin_api.Options
---@field private params jellyfin_api.get_endpoint_infoParams
local get_endpoint_info = {}
get_endpoint_info.__index = get_endpoint_info

---@param options jellyfin_api.Options
---@param parameters jellyfin_api.get_endpoint_infoParams
---@return jellyfin_api.get_endpoint_infoOperation
function get_endpoint_info.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, get_endpoint_info)
end

---@param request_context? jellyfin_api.RequestContext
---@return MluaHttpRequest
function get_endpoint_info:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/System/Endpoint", {}, {}
    R.parameters(params, {})
    R.authorize(options, {{["CustomAuthentication"] = {}, }, }, "GetEndpointInfo", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return jellyfin_api.models.EndPointInfo|jellyfin_api.models.EndPointInfo|jellyfin_api.models.EndPointInfo
function get_endpoint_info:parse_response(response)
    local status = response:status()
    local context = { operation_id = "GetEndpointInfo", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "application/json" then
            return R.json_response(self.options.json, response, context, V.check_EndPointInfo)
        end
        if context.content_type == "application/json; profile=\"CamelCase\"" then
            return R.json_response(self.options.json, response, context, V.check_EndPointInfo)
        end
        if context.content_type == "application/json; profile=\"PascalCase\"" then
            return R.json_response(self.options.json, response, context, V.check_EndPointInfo)
        end
        if R.media_matches(context.content_type, "application/json") then
            return R.json_response(self.options.json, response, context, V.check_EndPointInfo)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return get_endpoint_info
