local R = require("subsonic_api.runtime")
local V = require("subsonic_api.models")

---@class (exact) subsonic_api.get_scan_statusParams

---@class subsonic_api.get_scan_statusOperation
---@field private options subsonic_api.Options
---@field private params subsonic_api.get_scan_statusParams
local get_scan_status = {}
get_scan_status.__index = get_scan_status

---@param options subsonic_api.Options
---@param parameters subsonic_api.get_scan_statusParams
---@return subsonic_api.get_scan_statusOperation
function get_scan_status.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, get_scan_status)
end

---@param request_context? subsonic_api.RequestContext
---@return MluaHttpRequest
function get_scan_status:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/rest/getScanStatus", {}, {}
    R.parameters(params, {})
    R.authorize(options, {{["apiKeyAuth"] = {}, ["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["legacyPassword"] = {}, ["protocolVersion"] = {}, ["username"] = {}, }, {["clientName"] = {}, ["format"] = {}, ["protocolVersion"] = {}, ["salt"] = {}, ["token"] = {}, ["username"] = {}, }, }, "getScanStatus", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return subsonic_api.models.GetScanStatusResponse
function get_scan_status:parse_response(response)
    local status = response:status()
    local context = { operation_id = "getScanStatus", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "application/json" then
            return R.json_response(self.options.json, response, context, V.check_GetScanStatusResponse)
        end
        if R.media_matches(context.content_type, "application/json") then
            return R.json_response(self.options.json, response, context, V.check_GetScanStatusResponse)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return get_scan_status
