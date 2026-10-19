local R = require("jellyfin_api.runtime")
local V = require("jellyfin_api.models")

---@class (exact) jellyfin_api.stop_encoding_processParams
---@field device_id jellyfin_api.models.Schema265
---@field play_session_id jellyfin_api.models.Schema266

---@class jellyfin_api.stop_encoding_processOperation
---@field private options jellyfin_api.Options
---@field private params jellyfin_api.stop_encoding_processParams
local stop_encoding_process = {}
stop_encoding_process.__index = stop_encoding_process

---@param options jellyfin_api.Options
---@param parameters jellyfin_api.stop_encoding_processParams
---@return jellyfin_api.stop_encoding_processOperation
function stop_encoding_process.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, stop_encoding_process)
end

---@param request_context? jellyfin_api.RequestContext
---@return MluaHttpRequest
function stop_encoding_process:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/Videos/ActiveEncodings", {}, {}
    do
        local value = params["device_id"]
        if value ~= nil then
            R.validate(V.check_Schema265, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "deviceId", R.scalar(value) }
        else
            R.parameter(false, "missing device_id")
        end
    end
    do
        local value = params["play_session_id"]
        if value ~= nil then
            R.validate(V.check_Schema266, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "playSessionId", R.scalar(value) }
        else
            R.parameter(false, "missing play_session_id")
        end
    end
    R.parameters(params, {["device_id"] = true, ["play_session_id"] = true, })
    R.authorize(options, {{["CustomAuthentication"] = {}, }, }, "StopEncodingProcess", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("DELETE", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return nil
function stop_encoding_process:parse_response(response)
    local status = response:status()
    local context = { operation_id = "StopEncodingProcess", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 204 then
        response:close()
        return nil
    end
    return R.http_error(response, context)
end

return stop_encoding_process
