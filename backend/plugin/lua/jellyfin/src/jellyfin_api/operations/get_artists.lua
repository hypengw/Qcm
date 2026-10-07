local R = require("jellyfin_api.runtime")
local V = require("jellyfin_api.models")

---@class (exact) jellyfin_api.get_artistsParams
---@field min_community_rating? jellyfin_api.models.Schema1
---@field start_index? jellyfin_api.models.Schema2
---@field limit? jellyfin_api.models.Schema3
---@field search_term? jellyfin_api.models.Schema4
---@field parent_id? jellyfin_api.models.Schema5
---@field fields? jellyfin_api.models.Schema6
---@field exclude_item_types? jellyfin_api.models.Schema7
---@field include_item_types? jellyfin_api.models.Schema8
---@field filters? jellyfin_api.models.Schema9
---@field is_favorite? jellyfin_api.models.Schema10
---@field media_types? jellyfin_api.models.Schema11
---@field genres? jellyfin_api.models.Schema12
---@field genre_ids? jellyfin_api.models.Schema13
---@field official_ratings? jellyfin_api.models.Schema14
---@field tags? jellyfin_api.models.Schema15
---@field years? jellyfin_api.models.Schema16
---@field enable_user_data? jellyfin_api.models.Schema17
---@field image_type_limit? jellyfin_api.models.Schema18
---@field enable_image_types? jellyfin_api.models.Schema19
---@field person? jellyfin_api.models.Schema20
---@field person_ids? jellyfin_api.models.Schema21
---@field person_types? jellyfin_api.models.Schema22
---@field studios? jellyfin_api.models.Schema23
---@field studio_ids? jellyfin_api.models.Schema24
---@field user_id? jellyfin_api.models.Schema25
---@field name_starts_with_or_greater? jellyfin_api.models.Schema26
---@field name_starts_with? jellyfin_api.models.Schema27
---@field name_less_than? jellyfin_api.models.Schema28
---@field sort_by? jellyfin_api.models.Schema29
---@field sort_order? jellyfin_api.models.Schema30
---@field enable_images? jellyfin_api.models.Schema31
---@field enable_total_record_count? jellyfin_api.models.Schema32

---@class jellyfin_api.get_artistsOperation
---@field private options jellyfin_api.Options
---@field private params jellyfin_api.get_artistsParams
local get_artists = {}
get_artists.__index = get_artists

---@param options jellyfin_api.Options
---@param parameters jellyfin_api.get_artistsParams
---@return jellyfin_api.get_artistsOperation
function get_artists.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, get_artists)
end

---@param request_context? jellyfin_api.RequestContext
---@return MluaHttpRequest
function get_artists:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/Artists", {}, {}
    do
        local value = params["min_community_rating"]
        if value ~= nil then
            R.validate(V.check_Schema1, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "minCommunityRating", R.scalar(value) }
        end
    end
    do
        local value = params["start_index"]
        if value ~= nil then
            R.validate(V.check_Schema2, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "startIndex", R.scalar(value) }
        end
    end
    do
        local value = params["limit"]
        if value ~= nil then
            R.validate(V.check_Schema3, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "limit", R.scalar(value) }
        end
    end
    do
        local value = params["search_term"]
        if value ~= nil then
            R.validate(V.check_Schema4, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "searchTerm", R.scalar(value) }
        end
    end
    do
        local value = params["parent_id"]
        if value ~= nil then
            R.validate(V.check_Schema5, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "parentId", R.scalar(value) }
        end
    end
    do
        local value = params["fields"]
        if value ~= nil then
            R.validate(V.check_Schema6, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "fields", R.scalar(element) }
            end
        end
    end
    do
        local value = params["exclude_item_types"]
        if value ~= nil then
            R.validate(V.check_Schema7, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "excludeItemTypes", R.scalar(element) }
            end
        end
    end
    do
        local value = params["include_item_types"]
        if value ~= nil then
            R.validate(V.check_Schema8, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "includeItemTypes", R.scalar(element) }
            end
        end
    end
    do
        local value = params["filters"]
        if value ~= nil then
            R.validate(V.check_Schema9, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "filters", R.scalar(element) }
            end
        end
    end
    do
        local value = params["is_favorite"]
        if value ~= nil then
            R.validate(V.check_Schema10, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "isFavorite", R.scalar(value) }
        end
    end
    do
        local value = params["media_types"]
        if value ~= nil then
            R.validate(V.check_Schema11, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "mediaTypes", R.scalar(element) }
            end
        end
    end
    do
        local value = params["genres"]
        if value ~= nil then
            R.validate(V.check_Schema12, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "genres", R.scalar(element) }
            end
        end
    end
    do
        local value = params["genre_ids"]
        if value ~= nil then
            R.validate(V.check_Schema13, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "genreIds", R.scalar(element) }
            end
        end
    end
    do
        local value = params["official_ratings"]
        if value ~= nil then
            R.validate(V.check_Schema14, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "officialRatings", R.scalar(element) }
            end
        end
    end
    do
        local value = params["tags"]
        if value ~= nil then
            R.validate(V.check_Schema15, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "tags", R.scalar(element) }
            end
        end
    end
    do
        local value = params["years"]
        if value ~= nil then
            R.validate(V.check_Schema16, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "years", R.scalar(element) }
            end
        end
    end
    do
        local value = params["enable_user_data"]
        if value ~= nil then
            R.validate(V.check_Schema17, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "enableUserData", R.scalar(value) }
        end
    end
    do
        local value = params["image_type_limit"]
        if value ~= nil then
            R.validate(V.check_Schema18, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "imageTypeLimit", R.scalar(value) }
        end
    end
    do
        local value = params["enable_image_types"]
        if value ~= nil then
            R.validate(V.check_Schema19, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "enableImageTypes", R.scalar(element) }
            end
        end
    end
    do
        local value = params["person"]
        if value ~= nil then
            R.validate(V.check_Schema20, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "person", R.scalar(value) }
        end
    end
    do
        local value = params["person_ids"]
        if value ~= nil then
            R.validate(V.check_Schema21, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "personIds", R.scalar(element) }
            end
        end
    end
    do
        local value = params["person_types"]
        if value ~= nil then
            R.validate(V.check_Schema22, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "personTypes", R.scalar(element) }
            end
        end
    end
    do
        local value = params["studios"]
        if value ~= nil then
            R.validate(V.check_Schema23, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "studios", R.scalar(element) }
            end
        end
    end
    do
        local value = params["studio_ids"]
        if value ~= nil then
            R.validate(V.check_Schema24, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "studioIds", R.scalar(element) }
            end
        end
    end
    do
        local value = params["user_id"]
        if value ~= nil then
            R.validate(V.check_Schema25, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "userId", R.scalar(value) }
        end
    end
    do
        local value = params["name_starts_with_or_greater"]
        if value ~= nil then
            R.validate(V.check_Schema26, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "nameStartsWithOrGreater", R.scalar(value) }
        end
    end
    do
        local value = params["name_starts_with"]
        if value ~= nil then
            R.validate(V.check_Schema27, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "nameStartsWith", R.scalar(value) }
        end
    end
    do
        local value = params["name_less_than"]
        if value ~= nil then
            R.validate(V.check_Schema28, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "nameLessThan", R.scalar(value) }
        end
    end
    do
        local value = params["sort_by"]
        if value ~= nil then
            R.validate(V.check_Schema29, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "sortBy", R.scalar(element) }
            end
        end
    end
    do
        local value = params["sort_order"]
        if value ~= nil then
            R.validate(V.check_Schema30, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "sortOrder", R.scalar(element) }
            end
        end
    end
    do
        local value = params["enable_images"]
        if value ~= nil then
            R.validate(V.check_Schema31, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "enableImages", R.scalar(value) }
        end
    end
    do
        local value = params["enable_total_record_count"]
        if value ~= nil then
            R.validate(V.check_Schema32, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "enableTotalRecordCount", R.scalar(value) }
        end
    end
    R.parameters(params, {["min_community_rating"] = true, ["start_index"] = true, ["limit"] = true, ["search_term"] = true, ["parent_id"] = true, ["fields"] = true, ["exclude_item_types"] = true, ["include_item_types"] = true, ["filters"] = true, ["is_favorite"] = true, ["media_types"] = true, ["genres"] = true, ["genre_ids"] = true, ["official_ratings"] = true, ["tags"] = true, ["years"] = true, ["enable_user_data"] = true, ["image_type_limit"] = true, ["enable_image_types"] = true, ["person"] = true, ["person_ids"] = true, ["person_types"] = true, ["studios"] = true, ["studio_ids"] = true, ["user_id"] = true, ["name_starts_with_or_greater"] = true, ["name_starts_with"] = true, ["name_less_than"] = true, ["sort_by"] = true, ["sort_order"] = true, ["enable_images"] = true, ["enable_total_record_count"] = true, })
    R.authorize(options, {{["CustomAuthentication"] = {}, }, }, "GetArtists", query, headers)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return jellyfin_api.models.BaseItemDtoQueryResult|jellyfin_api.models.BaseItemDtoQueryResult|jellyfin_api.models.BaseItemDtoQueryResult
function get_artists:parse_response(response)
    local status = response:status()
    local context = { operation_id = "GetArtists", status = status, content_type = response:header_values('content-type')[1] }
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

return get_artists
