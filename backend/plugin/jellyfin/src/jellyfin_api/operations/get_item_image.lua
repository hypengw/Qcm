local R = require("jellyfin_api.runtime")
local V = require("jellyfin_api.models")

---@class (exact) jellyfin_api.get_item_imageParams
---@field item_id jellyfin_api.models.Schema205
---@field image_type jellyfin_api.models.Schema206
---@field max_width? jellyfin_api.models.Schema207
---@field max_height? jellyfin_api.models.Schema208
---@field width? jellyfin_api.models.Schema209
---@field height? jellyfin_api.models.Schema210
---@field quality? jellyfin_api.models.Schema211
---@field fill_width? jellyfin_api.models.Schema212
---@field fill_height? jellyfin_api.models.Schema213
---@field tag? jellyfin_api.models.Schema214
---@field format? jellyfin_api.models.Schema215
---@field percent_played? jellyfin_api.models.Schema216
---@field unplayed_count? jellyfin_api.models.Schema217
---@field blur? jellyfin_api.models.Schema218
---@field background_color? jellyfin_api.models.Schema219
---@field foreground_layer? jellyfin_api.models.Schema220
---@field image_index? jellyfin_api.models.Schema221

---@class jellyfin_api.get_item_imageOperation
---@field private options jellyfin_api.Options
---@field private params jellyfin_api.get_item_imageParams
local get_item_image = {}
get_item_image.__index = get_item_image

---@param options jellyfin_api.Options
---@param parameters jellyfin_api.get_item_imageParams
---@return jellyfin_api.get_item_imageOperation
function get_item_image.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, get_item_image)
end

---@param request_context? jellyfin_api.RequestContext
---@return MluaHttpRequest
function get_item_image:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/Items/{itemId}/Images/{imageType}", {}, {}
    do
        local value = params["item_id"]
        if value ~= nil then
            R.validate(V.check_Schema205, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            path = R.path_parameter(options.url, path, "itemId", value)
        else
            R.parameter(false, "missing item_id")
        end
    end
    do
        local value = params["image_type"]
        if value ~= nil then
            R.validate(V.check_Schema206, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            path = R.path_parameter(options.url, path, "imageType", value)
        else
            R.parameter(false, "missing image_type")
        end
    end
    do
        local value = params["max_width"]
        if value ~= nil then
            R.validate(V.check_Schema207, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "maxWidth", R.scalar(value) }
        end
    end
    do
        local value = params["max_height"]
        if value ~= nil then
            R.validate(V.check_Schema208, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "maxHeight", R.scalar(value) }
        end
    end
    do
        local value = params["width"]
        if value ~= nil then
            R.validate(V.check_Schema209, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "width", R.scalar(value) }
        end
    end
    do
        local value = params["height"]
        if value ~= nil then
            R.validate(V.check_Schema210, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "height", R.scalar(value) }
        end
    end
    do
        local value = params["quality"]
        if value ~= nil then
            R.validate(V.check_Schema211, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "quality", R.scalar(value) }
        end
    end
    do
        local value = params["fill_width"]
        if value ~= nil then
            R.validate(V.check_Schema212, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "fillWidth", R.scalar(value) }
        end
    end
    do
        local value = params["fill_height"]
        if value ~= nil then
            R.validate(V.check_Schema213, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "fillHeight", R.scalar(value) }
        end
    end
    do
        local value = params["tag"]
        if value ~= nil then
            R.validate(V.check_Schema214, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "tag", R.scalar(value) }
        end
    end
    do
        local value = params["format"]
        if value ~= nil then
            R.validate(V.check_Schema215, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "format", R.scalar(value) }
        end
    end
    do
        local value = params["percent_played"]
        if value ~= nil then
            R.validate(V.check_Schema216, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "percentPlayed", R.scalar(value) }
        end
    end
    do
        local value = params["unplayed_count"]
        if value ~= nil then
            R.validate(V.check_Schema217, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "unplayedCount", R.scalar(value) }
        end
    end
    do
        local value = params["blur"]
        if value ~= nil then
            R.validate(V.check_Schema218, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "blur", R.scalar(value) }
        end
    end
    do
        local value = params["background_color"]
        if value ~= nil then
            R.validate(V.check_Schema219, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "backgroundColor", R.scalar(value) }
        end
    end
    do
        local value = params["foreground_layer"]
        if value ~= nil then
            R.validate(V.check_Schema220, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "foregroundLayer", R.scalar(value) }
        end
    end
    do
        local value = params["image_index"]
        if value ~= nil then
            R.validate(V.check_Schema221, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "imageIndex", R.scalar(value) }
        end
    end
    R.parameters(params, {["item_id"] = true, ["image_type"] = true, ["max_width"] = true, ["max_height"] = true, ["width"] = true, ["height"] = true, ["quality"] = true, ["fill_width"] = true, ["fill_height"] = true, ["tag"] = true, ["format"] = true, ["percent_played"] = true, ["unplayed_count"] = true, ["blur"] = true, ["background_color"] = true, ["foreground_layer"] = true, ["image_index"] = true, })
    R.authorize(options, {}, "GetItemImage", query, headers, nil)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', '*/*' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    builder = builder:stream { headers_timeout_ms = options.headers_timeout_ms or 30000 }
    return builder:build()
end

---@param response MluaHttpResponse
---@return MluaHttpResponse
function get_item_image:parse_response(response)
    local status = response:status()
    local context = { operation_id = "GetItemImage", status = status, content_type = response:header_values('content-type')[1] }
    if status < 200 or status >= 300 then return R.http_error(response, context) end
    if status == 200 then
        if context.content_type == "image/*" then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema222)
        end
        if R.media_matches(context.content_type, "image/*") then
            if not R.is_json(context.content_type) then return response end
            return R.json_response(self.options.json, response, context, V.check_Schema222)
        end
        return R.media_error(response, context)
    end
    return R.http_error(response, context)
end

return get_item_image
