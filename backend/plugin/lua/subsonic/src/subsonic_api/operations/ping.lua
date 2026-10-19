local R = require("subsonic_api.runtime")
local V = require("subsonic_api.models")

---@class (exact) subsonic_api.pingParams

---@class subsonic_api.pingOperation
---@field private options subsonic_api.Options
---@field private params subsonic_api.pingParams
local ping = {}
ping.__index = ping

---@param options subsonic_api.Options
---@param parameters subsonic_api.pingParams
---@return subsonic_api.pingOperation
function ping.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, ping)
end

---@param request_context? subsonic_api.RequestContext
---@return MluaHttpRequest
function ping:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/rest/ping", {}, {}
    R.parameters(params, {})
    R.authorize(options, {{["apiKeyAuth"] = {}, ["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["legacyPassword"] = {}, ["protocolVersion"] = {}, ["username"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, ["salt"] = {}, ["token"] = {}, ["username"] = {}, }, }, "ping", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return subsonic_api.models.SubsonicResponse
function ping:parse_response(response)
    local status = response:status()
    local context = { operation_id = "ping", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "application/json" then
            return R.json_response(self.options.json, response, context, V.check_SubsonicResponse)
        end
        if R.media_matches(context.content_type, "application/json") then
            return R.json_response(self.options.json, response, context, V.check_SubsonicResponse)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return ping
