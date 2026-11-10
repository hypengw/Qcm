local R = require("jellyfin_api.runtime")
local V = require("jellyfin_api.models")

---@class (exact) jellyfin_api.get_user_viewsParams
---@field user_id? jellyfin_api.models.Schema258
---@field include_external_content? jellyfin_api.models.Schema259
---@field preset_views? jellyfin_api.models.Schema260
---@field include_hidden? jellyfin_api.models.Schema261

---@class jellyfin_api.get_user_viewsOperation
---@field private options jellyfin_api.Options
---@field private params jellyfin_api.get_user_viewsParams
local get_user_views = {}
get_user_views.__index = get_user_views

---@param options jellyfin_api.Options
---@param parameters jellyfin_api.get_user_viewsParams
---@return jellyfin_api.get_user_viewsOperation
function get_user_views.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, get_user_views)
end

---@param request_context? jellyfin_api.RequestContext
---@return MluaHttpRequest
function get_user_views:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/UserViews", {}, {}
    do
        local value = params["user_id"]
        if value ~= nil then
            R.validate(V.check_Schema258, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "userId", R.scalar(value) }
        end
    end
    do
        local value = params["include_external_content"]
        if value ~= nil then
            R.validate(V.check_Schema259, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "includeExternalContent", R.scalar(value) }
        end
    end
    do
        local value = params["preset_views"]
        if value ~= nil then
            R.validate(V.check_Schema260, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "presetViews", R.scalar(element) }
            end
        end
    end
    do
        local value = params["include_hidden"]
        if value ~= nil then
            R.validate(V.check_Schema261, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "includeHidden", R.scalar(value) }
        end
    end
    R.parameters(params, {["user_id"] = true, ["include_external_content"] = true, ["preset_views"] = true, ["include_hidden"] = true, })
    R.authorize(options, {{["CustomAuthentication"] = {}, }, }, "GetUserViews", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return jellyfin_api.models.BaseItemDtoQueryResult|jellyfin_api.models.BaseItemDtoQueryResult|jellyfin_api.models.BaseItemDtoQueryResult
function get_user_views:parse_response(response)
    local status = response:status()
    local context = { operation_id = "GetUserViews", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "application/json" then
            return R.json_response(self.options.json, response, context, V.check_BaseItemDtoQueryResult)
        end
        if context.content_type == "application/json; profile=\"CamelCase\"" then
            return R.json_response(self.options.json, response, context, V.check_BaseItemDtoQueryResult)
        end
        if context.content_type == "application/json; profile=\"PascalCase\"" then
            return R.json_response(self.options.json, response, context, V.check_BaseItemDtoQueryResult)
        end
        if R.media_matches(context.content_type, "application/json") then
            return R.json_response(self.options.json, response, context, V.check_BaseItemDtoQueryResult)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return get_user_views
