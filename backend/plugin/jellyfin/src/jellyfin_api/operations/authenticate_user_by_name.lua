local R = require("jellyfin_api.runtime")
local V = require("jellyfin_api.models")

---@class (exact) jellyfin_api.authenticate_user_by_nameParams
---@field body jellyfin_api.models.Schema263

---@class jellyfin_api.authenticate_user_by_nameOperation
---@field private options jellyfin_api.Options
---@field private params jellyfin_api.authenticate_user_by_nameParams
local authenticate_user_by_name = {}
authenticate_user_by_name.__index = authenticate_user_by_name

---@param options jellyfin_api.Options
---@param parameters jellyfin_api.authenticate_user_by_nameParams
---@return jellyfin_api.authenticate_user_by_nameOperation
function authenticate_user_by_name.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, authenticate_user_by_name)
end

---@param request_context? jellyfin_api.RequestContext
---@return MluaHttpRequest
function authenticate_user_by_name:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/Users/AuthenticateByName", {}, {}
    R.parameters(params, { body = true, })
    R.authorize(options, {}, "AuthenticateUserByName", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("POST", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    if params.body ~= nil then
        R.validate(V.check_Schema263, J, params.body)
        builder = builder:json(params.body)
    else
        R.parameter(false, 'missing request body')
    end
    return builder:build()
end

---@param response MluaHttpResponse
---@return jellyfin_api.models.AuthenticationResult|jellyfin_api.models.AuthenticationResult|jellyfin_api.models.AuthenticationResult
function authenticate_user_by_name:parse_response(response)
    local status = response:status()
    local context = { operation_id = "AuthenticateUserByName", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "application/json" then
            return R.json_response(self.options.json, response, context, V.check_AuthenticationResult)
        end
        if context.content_type == "application/json; profile=\"CamelCase\"" then
            return R.json_response(self.options.json, response, context, V.check_AuthenticationResult)
        end
        if context.content_type == "application/json; profile=\"PascalCase\"" then
            return R.json_response(self.options.json, response, context, V.check_AuthenticationResult)
        end
        if R.media_matches(context.content_type, "application/json") then
            return R.json_response(self.options.json, response, context, V.check_AuthenticationResult)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return authenticate_user_by_name
