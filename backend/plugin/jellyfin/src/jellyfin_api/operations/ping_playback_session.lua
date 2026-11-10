local R = require("jellyfin_api.runtime")
local V = require("jellyfin_api.models")

---@class (exact) jellyfin_api.ping_playback_sessionParams
---@field play_session_id jellyfin_api.models.Schema253

---@class jellyfin_api.ping_playback_sessionOperation
---@field private options jellyfin_api.Options
---@field private params jellyfin_api.ping_playback_sessionParams
local ping_playback_session = {}
ping_playback_session.__index = ping_playback_session

---@param options jellyfin_api.Options
---@param parameters jellyfin_api.ping_playback_sessionParams
---@return jellyfin_api.ping_playback_sessionOperation
function ping_playback_session.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, ping_playback_session)
end

---@param request_context? jellyfin_api.RequestContext
---@return MluaHttpRequest
function ping_playback_session:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/Sessions/Playing/Ping", {}, {}
    do
        local value = params["play_session_id"]
        if value ~= nil then
            R.validate(V.check_Schema253, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "playSessionId", R.scalar(value) }
        else
            R.parameter(false, "missing play_session_id")
        end
    end
    R.parameters(params, {["play_session_id"] = true, })
    R.authorize(options, {{["CustomAuthentication"] = {}, }, }, "PingPlaybackSession", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("POST", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return nil
function ping_playback_session:parse_response(response)
    local status = response:status()
    local context = { operation_id = "PingPlaybackSession", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 204 then
        response:close()
        return nil
    end
    return R.http_error(response, context)
end

return ping_playback_session
