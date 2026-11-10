local R = require("subsonic_api.runtime")
local V = require("subsonic_api.models")

---@class (exact) subsonic_api.post_get_lyrics_by_song_idParams
---@field body subsonic_api.models.Schema29

---@class subsonic_api.post_get_lyrics_by_song_idOperation
---@field private options subsonic_api.Options
---@field private params subsonic_api.post_get_lyrics_by_song_idParams
local post_get_lyrics_by_song_id = {}
post_get_lyrics_by_song_id.__index = post_get_lyrics_by_song_id

---@param options subsonic_api.Options
---@param parameters subsonic_api.post_get_lyrics_by_song_idParams
---@return subsonic_api.post_get_lyrics_by_song_idOperation
function post_get_lyrics_by_song_id.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, post_get_lyrics_by_song_id)
end

---@param request_context? subsonic_api.RequestContext
---@return MluaHttpRequest
function post_get_lyrics_by_song_id:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/rest/getLyricsBySongId", {}, {}
    local form = {}
    if params.body ~= nil then
        R.validate(V.check_Schema29, J, params.body)
        form = R.form(J, params.body)
    else
        R.parameter(false, 'missing request body')
    end
    R.parameters(params, { body = true, })
    R.authorize(options, {{["apiKeyAuth"] = {}, ["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["legacyPassword"] = {}, ["protocolVersion"] = {}, ["username"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, ["salt"] = {}, ["token"] = {}, ["username"] = {}, }, }, "postGetLyricsBySongId", query, headers, form)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("POST", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    builder = builder:form_pairs(form)
    return builder:build()
end

---@param response MluaHttpResponse
---@return subsonic_api.models.GetLyricsBySongIdResponse
function post_get_lyrics_by_song_id:parse_response(response)
    local status = response:status()
    local context = { operation_id = "postGetLyricsBySongId", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "application/json" then
            return R.json_response(self.options.json, response, context, V.check_GetLyricsBySongIdResponse)
        end
        if R.media_matches(context.content_type, "application/json") then
            return R.json_response(self.options.json, response, context, V.check_GetLyricsBySongIdResponse)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return post_get_lyrics_by_song_id
