local R = require("subsonic_api.runtime")
local V = require("subsonic_api.models")

---@class (exact) subsonic_api.get_random_songsParams
---@field size? subsonic_api.models.Schema36
---@field genre? subsonic_api.models.Schema37
---@field from_year? subsonic_api.models.Schema38
---@field to_year? subsonic_api.models.Schema39
---@field music_folder_id? subsonic_api.models.Schema40

---@class subsonic_api.get_random_songsOperation
---@field private options subsonic_api.Options
---@field private params subsonic_api.get_random_songsParams
local get_random_songs = {}
get_random_songs.__index = get_random_songs

---@param options subsonic_api.Options
---@param parameters subsonic_api.get_random_songsParams
---@return subsonic_api.get_random_songsOperation
function get_random_songs.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, get_random_songs)
end

---@param request_context? subsonic_api.RequestContext
---@return MluaHttpRequest
function get_random_songs:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/rest/getRandomSongs", {}, {}
    do
        local value = params["size"]
        if value ~= nil then
            R.validate(V.check_Schema36, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "size", R.scalar(value) }
        end
    end
    do
        local value = params["genre"]
        if value ~= nil then
            R.validate(V.check_Schema37, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "genre", R.scalar(value) }
        end
    end
    do
        local value = params["from_year"]
        if value ~= nil then
            R.validate(V.check_Schema38, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "fromYear", R.scalar(value) }
        end
    end
    do
        local value = params["to_year"]
        if value ~= nil then
            R.validate(V.check_Schema39, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "toYear", R.scalar(value) }
        end
    end
    do
        local value = params["music_folder_id"]
        if value ~= nil then
            R.validate(V.check_Schema40, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "musicFolderId", R.scalar(value) }
        end
    end
    R.parameters(params, {["size"] = true, ["genre"] = true, ["from_year"] = true, ["to_year"] = true, ["music_folder_id"] = true, })
    R.authorize(options, {{["apiKeyAuth"] = {}, ["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["legacyPassword"] = {}, ["protocolVersion"] = {}, ["username"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, ["salt"] = {}, ["token"] = {}, ["username"] = {}, }, }, "getRandomSongs", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return subsonic_api.models.GetRandomSongsResponse
function get_random_songs:parse_response(response)
    local status = response:status()
    local context = { operation_id = "getRandomSongs", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "application/json" then
            return R.json_response(self.options.json, response, context, V.check_GetRandomSongsResponse)
        end
        if R.media_matches(context.content_type, "application/json") then
            return R.json_response(self.options.json, response, context, V.check_GetRandomSongsResponse)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return get_random_songs
