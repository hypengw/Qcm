local R = require("jellyfin_api.runtime")
local V = require("jellyfin_api.models")

---@class (exact) jellyfin_api.get_playlist_itemsParams
---@field playlist_id jellyfin_api.models.Schema226
---@field user_id? jellyfin_api.models.Schema227
---@field start_index? jellyfin_api.models.Schema228
---@field limit? jellyfin_api.models.Schema229
---@field fields? jellyfin_api.models.Schema230
---@field enable_images? jellyfin_api.models.Schema231
---@field enable_user_data? jellyfin_api.models.Schema232
---@field image_type_limit? jellyfin_api.models.Schema233
---@field enable_image_types? jellyfin_api.models.Schema234

---@class jellyfin_api.get_playlist_itemsOperation
---@field private options jellyfin_api.Options
---@field private params jellyfin_api.get_playlist_itemsParams
local get_playlist_items = {}
get_playlist_items.__index = get_playlist_items

---@param options jellyfin_api.Options
---@param parameters jellyfin_api.get_playlist_itemsParams
---@return jellyfin_api.get_playlist_itemsOperation
function get_playlist_items.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, get_playlist_items)
end

---@param request_context? jellyfin_api.RequestContext
---@return MluaHttpRequest
function get_playlist_items:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/Playlists/{playlistId}/Items", {}, {}
    do
        local value = params["playlist_id"]
        if value ~= nil then
            R.validate(V.check_Schema226, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            path = R.path_parameter(options.url, path, "playlistId", value)
        else
            R.parameter(false, "missing playlist_id")
        end
    end
    do
        local value = params["user_id"]
        if value ~= nil then
            R.validate(V.check_Schema227, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "userId", R.scalar(value) }
        end
    end
    do
        local value = params["start_index"]
        if value ~= nil then
            R.validate(V.check_Schema228, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "startIndex", R.scalar(value) }
        end
    end
    do
        local value = params["limit"]
        if value ~= nil then
            R.validate(V.check_Schema229, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "limit", R.scalar(value) }
        end
    end
    do
        local value = params["fields"]
        if value ~= nil then
            R.validate(V.check_Schema230, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "fields", R.scalar(element) }
            end
        end
    end
    do
        local value = params["enable_images"]
        if value ~= nil then
            R.validate(V.check_Schema231, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "enableImages", R.scalar(value) }
        end
    end
    do
        local value = params["enable_user_data"]
        if value ~= nil then
            R.validate(V.check_Schema232, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "enableUserData", R.scalar(value) }
        end
    end
    do
        local value = params["image_type_limit"]
        if value ~= nil then
            R.validate(V.check_Schema233, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "imageTypeLimit", R.scalar(value) }
        end
    end
    do
        local value = params["enable_image_types"]
        if value ~= nil then
            R.validate(V.check_Schema234, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "enableImageTypes", R.scalar(element) }
            end
        end
    end
    R.parameters(params, {["playlist_id"] = true, ["user_id"] = true, ["start_index"] = true, ["limit"] = true, ["fields"] = true, ["enable_images"] = true, ["enable_user_data"] = true, ["image_type_limit"] = true, ["enable_image_types"] = true, })
    R.authorize(options, {{["CustomAuthentication"] = {}, }, }, "GetPlaylistItems", query, headers)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return jellyfin_api.models.BaseItemDtoQueryResult|jellyfin_api.models.BaseItemDtoQueryResult|jellyfin_api.models.BaseItemDtoQueryResult
function get_playlist_items:parse_response(response)
    local status = response:status()
    local context = { operation_id = "GetPlaylistItems", status = status, content_type = response:header_values('content-type')[1] }
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

return get_playlist_items
