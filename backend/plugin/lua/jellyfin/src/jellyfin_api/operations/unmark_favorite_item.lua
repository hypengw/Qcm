local R = require("jellyfin_api.runtime")
local V = require("jellyfin_api.models")

---@class (exact) jellyfin_api.unmark_favorite_itemParams
---@field user_id? jellyfin_api.models.Schema254
---@field item_id jellyfin_api.models.Schema255

---@class jellyfin_api.unmark_favorite_itemOperation
---@field private options jellyfin_api.Options
---@field private params jellyfin_api.unmark_favorite_itemParams
local unmark_favorite_item = {}
unmark_favorite_item.__index = unmark_favorite_item

---@param options jellyfin_api.Options
---@param parameters jellyfin_api.unmark_favorite_itemParams
---@return jellyfin_api.unmark_favorite_itemOperation
function unmark_favorite_item.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, unmark_favorite_item)
end

---@param request_context? jellyfin_api.RequestContext
---@return MluaHttpRequest
function unmark_favorite_item:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/UserFavoriteItems/{itemId}", {}, {}
    do
        local value = params["user_id"]
        if value ~= nil then
            R.validate(V.check_Schema254, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "userId", R.scalar(value) }
        end
    end
    do
        local value = params["item_id"]
        if value ~= nil then
            R.validate(V.check_Schema255, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            path = R.path_parameter(options.url, path, "itemId", value)
        else
            R.parameter(false, "missing item_id")
        end
    end
    R.parameters(params, {["user_id"] = true, ["item_id"] = true, })
    R.authorize(options, {{["CustomAuthentication"] = {}, }, }, "UnmarkFavoriteItem", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("DELETE", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return jellyfin_api.models.UserItemDataDto|jellyfin_api.models.UserItemDataDto|jellyfin_api.models.UserItemDataDto
function unmark_favorite_item:parse_response(response)
    local status = response:status()
    local context = { operation_id = "UnmarkFavoriteItem", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "application/json" then
            return R.json_response(self.options.json, response, context, V.check_UserItemDataDto)
        end
        if context.content_type == "application/json; profile=\"CamelCase\"" then
            return R.json_response(self.options.json, response, context, V.check_UserItemDataDto)
        end
        if context.content_type == "application/json; profile=\"PascalCase\"" then
            return R.json_response(self.options.json, response, context, V.check_UserItemDataDto)
        end
        if R.media_matches(context.content_type, "application/json") then
            return R.json_response(self.options.json, response, context, V.check_UserItemDataDto)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return unmark_favorite_item
