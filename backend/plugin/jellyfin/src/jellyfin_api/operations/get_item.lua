local R = require("jellyfin_api.runtime")
local V = require("jellyfin_api.models")

---@class (exact) jellyfin_api.get_itemParams
---@field user_id? jellyfin_api.models.Schema203
---@field item_id jellyfin_api.models.Schema204

---@class jellyfin_api.get_itemOperation
---@field private options jellyfin_api.Options
---@field private params jellyfin_api.get_itemParams
local get_item = {}
get_item.__index = get_item

---@param options jellyfin_api.Options
---@param parameters jellyfin_api.get_itemParams
---@return jellyfin_api.get_itemOperation
function get_item.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, get_item)
end

---@param request_context? jellyfin_api.RequestContext
---@return MluaHttpRequest
function get_item:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/Items/{itemId}", {}, {}
    do
        local value = params["user_id"]
        if value ~= nil then
            R.validate(V.check_Schema203, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "userId", R.scalar(value) }
        end
    end
    do
        local value = params["item_id"]
        if value ~= nil then
            R.validate(V.check_Schema204, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            path = R.path_parameter(options.url, path, "itemId", value)
        else
            R.parameter(false, "missing item_id")
        end
    end
    R.parameters(params, {["user_id"] = true, ["item_id"] = true, })
    R.authorize(options, {{["CustomAuthentication"] = {}, }, }, "GetItem", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return jellyfin_api.models.BaseItemDto|jellyfin_api.models.BaseItemDto|jellyfin_api.models.BaseItemDto
function get_item:parse_response(response)
    local status = response:status()
    local context = { operation_id = "GetItem", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "application/json" then
            return R.json_response(self.options.json, response, context, V.check_BaseItemDto)
        end
        if context.content_type == "application/json; profile=\"CamelCase\"" then
            return R.json_response(self.options.json, response, context, V.check_BaseItemDto)
        end
        if context.content_type == "application/json; profile=\"PascalCase\"" then
            return R.json_response(self.options.json, response, context, V.check_BaseItemDto)
        end
        if R.media_matches(context.content_type, "application/json") then
            return R.json_response(self.options.json, response, context, V.check_BaseItemDto)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return get_item
