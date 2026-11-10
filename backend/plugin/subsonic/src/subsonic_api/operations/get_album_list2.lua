local R = require("subsonic_api.runtime")
local V = require("subsonic_api.models")

---@class (exact) subsonic_api.get_album_list2Params
---@field type subsonic_api.models.AlbumListType
---@field size? subsonic_api.models.Schema3
---@field offset? subsonic_api.models.Schema4
---@field from_year? subsonic_api.models.Schema5
---@field to_year? subsonic_api.models.Schema6
---@field genre? subsonic_api.models.Schema7
---@field music_folder_id? subsonic_api.models.Schema8

---@class subsonic_api.get_album_list2Operation
---@field private options subsonic_api.Options
---@field private params subsonic_api.get_album_list2Params
local get_album_list2 = {}
get_album_list2.__index = get_album_list2

---@param options subsonic_api.Options
---@param parameters subsonic_api.get_album_list2Params
---@return subsonic_api.get_album_list2Operation
function get_album_list2.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, get_album_list2)
end

---@param request_context? subsonic_api.RequestContext
---@return MluaHttpRequest
function get_album_list2:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/rest/getAlbumList2", {}, {}
    do
        local value = params["type"]
        if value ~= nil then
            R.validate(V.check_AlbumListType, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "type", R.scalar(value) }
        else
            R.parameter(false, "missing type")
        end
    end
    do
        local value = params["size"]
        if value ~= nil then
            R.validate(V.check_Schema3, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "size", R.scalar(value) }
        end
    end
    do
        local value = params["offset"]
        if value ~= nil then
            R.validate(V.check_Schema4, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "offset", R.scalar(value) }
        end
    end
    do
        local value = params["from_year"]
        if value ~= nil then
            R.validate(V.check_Schema5, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "fromYear", R.scalar(value) }
        end
    end
    do
        local value = params["to_year"]
        if value ~= nil then
            R.validate(V.check_Schema6, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "toYear", R.scalar(value) }
        end
    end
    do
        local value = params["genre"]
        if value ~= nil then
            R.validate(V.check_Schema7, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "genre", R.scalar(value) }
        end
    end
    do
        local value = params["music_folder_id"]
        if value ~= nil then
            R.validate(V.check_Schema8, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "musicFolderId", R.scalar(value) }
        end
    end
    R.parameters(params, {["type"] = true, ["size"] = true, ["offset"] = true, ["from_year"] = true, ["to_year"] = true, ["genre"] = true, ["music_folder_id"] = true, })
    R.authorize(options, {{["apiKeyAuth"] = {}, ["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["legacyPassword"] = {}, ["protocolVersion"] = {}, ["username"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, ["salt"] = {}, ["token"] = {}, ["username"] = {}, }, }, "getAlbumList2", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return subsonic_api.models.GetAlbumList2Response
function get_album_list2:parse_response(response)
    local status = response:status()
    local context = { operation_id = "getAlbumList2", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "application/json" then
            return R.json_response(self.options.json, response, context, V.check_GetAlbumList2Response)
        end
        if R.media_matches(context.content_type, "application/json") then
            return R.json_response(self.options.json, response, context, V.check_GetAlbumList2Response)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return get_album_list2
