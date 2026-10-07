local R = require("jellyfin_api.runtime")
local V = require("jellyfin_api.models")

---@class (exact) jellyfin_api.get_album_artistsParams
---@field min_community_rating? jellyfin_api.models.Schema33
---@field start_index? jellyfin_api.models.Schema34
---@field limit? jellyfin_api.models.Schema35
---@field search_term? jellyfin_api.models.Schema36
---@field parent_id? jellyfin_api.models.Schema37
---@field fields? jellyfin_api.models.Schema38
---@field exclude_item_types? jellyfin_api.models.Schema39
---@field include_item_types? jellyfin_api.models.Schema40
---@field filters? jellyfin_api.models.Schema41
---@field is_favorite? jellyfin_api.models.Schema42
---@field media_types? jellyfin_api.models.Schema43
---@field genres? jellyfin_api.models.Schema44
---@field genre_ids? jellyfin_api.models.Schema45
---@field official_ratings? jellyfin_api.models.Schema46
---@field tags? jellyfin_api.models.Schema47
---@field years? jellyfin_api.models.Schema48
---@field enable_user_data? jellyfin_api.models.Schema49
---@field image_type_limit? jellyfin_api.models.Schema50
---@field enable_image_types? jellyfin_api.models.Schema51
---@field person? jellyfin_api.models.Schema52
---@field person_ids? jellyfin_api.models.Schema53
---@field person_types? jellyfin_api.models.Schema54
---@field studios? jellyfin_api.models.Schema55
---@field studio_ids? jellyfin_api.models.Schema56
---@field user_id? jellyfin_api.models.Schema57
---@field name_starts_with_or_greater? jellyfin_api.models.Schema58
---@field name_starts_with? jellyfin_api.models.Schema59
---@field name_less_than? jellyfin_api.models.Schema60
---@field sort_by? jellyfin_api.models.Schema61
---@field sort_order? jellyfin_api.models.Schema62
---@field enable_images? jellyfin_api.models.Schema63
---@field enable_total_record_count? jellyfin_api.models.Schema64

---@class jellyfin_api.get_album_artistsOperation
---@field private options jellyfin_api.Options
---@field private params jellyfin_api.get_album_artistsParams
local get_album_artists = {}
get_album_artists.__index = get_album_artists

---@param options jellyfin_api.Options
---@param parameters jellyfin_api.get_album_artistsParams
---@return jellyfin_api.get_album_artistsOperation
function get_album_artists.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, get_album_artists)
end

---@param request_context? jellyfin_api.RequestContext
---@return MluaHttpRequest
function get_album_artists:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/Artists/AlbumArtists", {}, {}
    do
        local value = params["min_community_rating"]
        if value ~= nil then
            R.validate(V.check_Schema33, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "minCommunityRating", R.scalar(value) }
        end
    end
    do
        local value = params["start_index"]
        if value ~= nil then
            R.validate(V.check_Schema34, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "startIndex", R.scalar(value) }
        end
    end
    do
        local value = params["limit"]
        if value ~= nil then
            R.validate(V.check_Schema35, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "limit", R.scalar(value) }
        end
    end
    do
        local value = params["search_term"]
        if value ~= nil then
            R.validate(V.check_Schema36, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "searchTerm", R.scalar(value) }
        end
    end
    do
        local value = params["parent_id"]
        if value ~= nil then
            R.validate(V.check_Schema37, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "parentId", R.scalar(value) }
        end
    end
    do
        local value = params["fields"]
        if value ~= nil then
            R.validate(V.check_Schema38, J, value)
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
            R.validate(V.check_Schema39, J, value)
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
            R.validate(V.check_Schema40, J, value)
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
            R.validate(V.check_Schema41, J, value)
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
            R.validate(V.check_Schema42, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "isFavorite", R.scalar(value) }
        end
    end
    do
        local value = params["media_types"]
        if value ~= nil then
            R.validate(V.check_Schema43, J, value)
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
            R.validate(V.check_Schema44, J, value)
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
            R.validate(V.check_Schema45, J, value)
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
            R.validate(V.check_Schema46, J, value)
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
            R.validate(V.check_Schema47, J, value)
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
            R.validate(V.check_Schema48, J, value)
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
            R.validate(V.check_Schema49, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "enableUserData", R.scalar(value) }
        end
    end
    do
        local value = params["image_type_limit"]
        if value ~= nil then
            R.validate(V.check_Schema50, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "imageTypeLimit", R.scalar(value) }
        end
    end
    do
        local value = params["enable_image_types"]
        if value ~= nil then
            R.validate(V.check_Schema51, J, value)
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
            R.validate(V.check_Schema52, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "person", R.scalar(value) }
        end
    end
    do
        local value = params["person_ids"]
        if value ~= nil then
            R.validate(V.check_Schema53, J, value)
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
            R.validate(V.check_Schema54, J, value)
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
            R.validate(V.check_Schema55, J, value)
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
            R.validate(V.check_Schema56, J, value)
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
            R.validate(V.check_Schema57, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "userId", R.scalar(value) }
        end
    end
    do
        local value = params["name_starts_with_or_greater"]
        if value ~= nil then
            R.validate(V.check_Schema58, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "nameStartsWithOrGreater", R.scalar(value) }
        end
    end
    do
        local value = params["name_starts_with"]
        if value ~= nil then
            R.validate(V.check_Schema59, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "nameStartsWith", R.scalar(value) }
        end
    end
    do
        local value = params["name_less_than"]
        if value ~= nil then
            R.validate(V.check_Schema60, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "nameLessThan", R.scalar(value) }
        end
    end
    do
        local value = params["sort_by"]
        if value ~= nil then
            R.validate(V.check_Schema61, J, value)
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
            R.validate(V.check_Schema62, J, value)
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
            R.validate(V.check_Schema63, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "enableImages", R.scalar(value) }
        end
    end
    do
        local value = params["enable_total_record_count"]
        if value ~= nil then
            R.validate(V.check_Schema64, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "enableTotalRecordCount", R.scalar(value) }
        end
    end
    R.parameters(params, {["min_community_rating"] = true, ["start_index"] = true, ["limit"] = true, ["search_term"] = true, ["parent_id"] = true, ["fields"] = true, ["exclude_item_types"] = true, ["include_item_types"] = true, ["filters"] = true, ["is_favorite"] = true, ["media_types"] = true, ["genres"] = true, ["genre_ids"] = true, ["official_ratings"] = true, ["tags"] = true, ["years"] = true, ["enable_user_data"] = true, ["image_type_limit"] = true, ["enable_image_types"] = true, ["person"] = true, ["person_ids"] = true, ["person_types"] = true, ["studios"] = true, ["studio_ids"] = true, ["user_id"] = true, ["name_starts_with_or_greater"] = true, ["name_starts_with"] = true, ["name_less_than"] = true, ["sort_by"] = true, ["sort_order"] = true, ["enable_images"] = true, ["enable_total_record_count"] = true, })
    R.authorize(options, {{["CustomAuthentication"] = {}, }, }, "GetAlbumArtists", query, headers)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return jellyfin_api.models.BaseItemDtoQueryResult|jellyfin_api.models.BaseItemDtoQueryResult|jellyfin_api.models.BaseItemDtoQueryResult
function get_album_artists:parse_response(response)
    local status = response:status()
    local context = { operation_id = "GetAlbumArtists", status = status, content_type = response:header_values('content-type')[1] }
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

return get_album_artists
