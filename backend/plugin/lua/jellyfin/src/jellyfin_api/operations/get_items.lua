local R = require("jellyfin_api.runtime")
local V = require("jellyfin_api.models")

---@class (exact) jellyfin_api.get_itemsParams
---@field user_id? jellyfin_api.models.Schema117
---@field max_official_rating? jellyfin_api.models.Schema118
---@field has_theme_song? jellyfin_api.models.Schema119
---@field has_theme_video? jellyfin_api.models.Schema120
---@field has_subtitles? jellyfin_api.models.Schema121
---@field has_special_feature? jellyfin_api.models.Schema122
---@field has_trailer? jellyfin_api.models.Schema123
---@field adjacent_to? jellyfin_api.models.Schema124
---@field index_number? jellyfin_api.models.Schema125
---@field parent_index_number? jellyfin_api.models.Schema126
---@field has_parental_rating? jellyfin_api.models.Schema127
---@field is_hd? jellyfin_api.models.Schema128
---@field is4k? jellyfin_api.models.Schema129
---@field location_types? jellyfin_api.models.Schema130
---@field exclude_location_types? jellyfin_api.models.Schema131
---@field is_missing? jellyfin_api.models.Schema132
---@field is_unaired? jellyfin_api.models.Schema133
---@field min_community_rating? jellyfin_api.models.Schema134
---@field min_critic_rating? jellyfin_api.models.Schema135
---@field min_premiere_date? jellyfin_api.models.Schema136
---@field min_date_last_saved? jellyfin_api.models.Schema137
---@field min_date_last_saved_for_user? jellyfin_api.models.Schema138
---@field max_premiere_date? jellyfin_api.models.Schema139
---@field has_overview? jellyfin_api.models.Schema140
---@field has_imdb_id? jellyfin_api.models.Schema141
---@field has_tmdb_id? jellyfin_api.models.Schema142
---@field has_tvdb_id? jellyfin_api.models.Schema143
---@field is_movie? jellyfin_api.models.Schema144
---@field is_series? jellyfin_api.models.Schema145
---@field is_news? jellyfin_api.models.Schema146
---@field is_kids? jellyfin_api.models.Schema147
---@field is_sports? jellyfin_api.models.Schema148
---@field exclude_item_ids? jellyfin_api.models.Schema149
---@field start_index? jellyfin_api.models.Schema150
---@field limit? jellyfin_api.models.Schema151
---@field recursive? jellyfin_api.models.Schema152
---@field search_term? jellyfin_api.models.Schema153
---@field sort_order? jellyfin_api.models.Schema154
---@field parent_id? jellyfin_api.models.Schema155
---@field fields? jellyfin_api.models.Schema156
---@field exclude_item_types? jellyfin_api.models.Schema157
---@field include_item_types? jellyfin_api.models.Schema158
---@field filters? jellyfin_api.models.Schema159
---@field is_favorite? jellyfin_api.models.Schema160
---@field media_types? jellyfin_api.models.Schema161
---@field image_types? jellyfin_api.models.Schema162
---@field sort_by? jellyfin_api.models.Schema163
---@field is_played? jellyfin_api.models.Schema164
---@field genres? jellyfin_api.models.Schema165
---@field official_ratings? jellyfin_api.models.Schema166
---@field tags? jellyfin_api.models.Schema167
---@field years? jellyfin_api.models.Schema168
---@field enable_user_data? jellyfin_api.models.Schema169
---@field image_type_limit? jellyfin_api.models.Schema170
---@field enable_image_types? jellyfin_api.models.Schema171
---@field person? jellyfin_api.models.Schema172
---@field person_ids? jellyfin_api.models.Schema173
---@field person_types? jellyfin_api.models.Schema174
---@field studios? jellyfin_api.models.Schema175
---@field artists? jellyfin_api.models.Schema176
---@field exclude_artist_ids? jellyfin_api.models.Schema177
---@field artist_ids? jellyfin_api.models.Schema178
---@field album_artist_ids? jellyfin_api.models.Schema179
---@field contributing_artist_ids? jellyfin_api.models.Schema180
---@field albums? jellyfin_api.models.Schema181
---@field album_ids? jellyfin_api.models.Schema182
---@field ids? jellyfin_api.models.Schema183
---@field video_types? jellyfin_api.models.Schema184
---@field min_official_rating? jellyfin_api.models.Schema185
---@field is_locked? jellyfin_api.models.Schema186
---@field is_place_holder? jellyfin_api.models.Schema187
---@field has_official_rating? jellyfin_api.models.Schema188
---@field collapse_box_set_items? jellyfin_api.models.Schema189
---@field min_width? jellyfin_api.models.Schema190
---@field min_height? jellyfin_api.models.Schema191
---@field max_width? jellyfin_api.models.Schema192
---@field max_height? jellyfin_api.models.Schema193
---@field is3d? jellyfin_api.models.Schema194
---@field series_status? jellyfin_api.models.Schema195
---@field name_starts_with_or_greater? jellyfin_api.models.Schema196
---@field name_starts_with? jellyfin_api.models.Schema197
---@field name_less_than? jellyfin_api.models.Schema198
---@field studio_ids? jellyfin_api.models.Schema199
---@field genre_ids? jellyfin_api.models.Schema200
---@field enable_total_record_count? jellyfin_api.models.Schema201
---@field enable_images? jellyfin_api.models.Schema202

---@class jellyfin_api.get_itemsOperation
---@field private options jellyfin_api.Options
---@field private params jellyfin_api.get_itemsParams
local get_items = {}
get_items.__index = get_items

---@param options jellyfin_api.Options
---@param parameters jellyfin_api.get_itemsParams
---@return jellyfin_api.get_itemsOperation
function get_items.new(options, parameters)
    return setmetatable({ options = options, params = R.snapshot(options.json, parameters or {}) }, get_items)
end

---@param request_context? jellyfin_api.RequestContext
---@return MluaHttpRequest
function get_items:build_request(request_context)
    local options, params = self.options, self.params
    local J = options.json
    local path, query, headers = "/Items", {}, {}
    do
        local value = params["user_id"]
        if value ~= nil then
            R.validate(V.check_Schema117, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "userId", R.scalar(value) }
        end
    end
    do
        local value = params["max_official_rating"]
        if value ~= nil then
            R.validate(V.check_Schema118, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "maxOfficialRating", R.scalar(value) }
        end
    end
    do
        local value = params["has_theme_song"]
        if value ~= nil then
            R.validate(V.check_Schema119, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "hasThemeSong", R.scalar(value) }
        end
    end
    do
        local value = params["has_theme_video"]
        if value ~= nil then
            R.validate(V.check_Schema120, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "hasThemeVideo", R.scalar(value) }
        end
    end
    do
        local value = params["has_subtitles"]
        if value ~= nil then
            R.validate(V.check_Schema121, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "hasSubtitles", R.scalar(value) }
        end
    end
    do
        local value = params["has_special_feature"]
        if value ~= nil then
            R.validate(V.check_Schema122, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "hasSpecialFeature", R.scalar(value) }
        end
    end
    do
        local value = params["has_trailer"]
        if value ~= nil then
            R.validate(V.check_Schema123, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "hasTrailer", R.scalar(value) }
        end
    end
    do
        local value = params["adjacent_to"]
        if value ~= nil then
            R.validate(V.check_Schema124, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "adjacentTo", R.scalar(value) }
        end
    end
    do
        local value = params["index_number"]
        if value ~= nil then
            R.validate(V.check_Schema125, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "indexNumber", R.scalar(value) }
        end
    end
    do
        local value = params["parent_index_number"]
        if value ~= nil then
            R.validate(V.check_Schema126, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "parentIndexNumber", R.scalar(value) }
        end
    end
    do
        local value = params["has_parental_rating"]
        if value ~= nil then
            R.validate(V.check_Schema127, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "hasParentalRating", R.scalar(value) }
        end
    end
    do
        local value = params["is_hd"]
        if value ~= nil then
            R.validate(V.check_Schema128, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "isHd", R.scalar(value) }
        end
    end
    do
        local value = params["is4k"]
        if value ~= nil then
            R.validate(V.check_Schema129, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "is4K", R.scalar(value) }
        end
    end
    do
        local value = params["location_types"]
        if value ~= nil then
            R.validate(V.check_Schema130, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "locationTypes", R.scalar(element) }
            end
        end
    end
    do
        local value = params["exclude_location_types"]
        if value ~= nil then
            R.validate(V.check_Schema131, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "excludeLocationTypes", R.scalar(element) }
            end
        end
    end
    do
        local value = params["is_missing"]
        if value ~= nil then
            R.validate(V.check_Schema132, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "isMissing", R.scalar(value) }
        end
    end
    do
        local value = params["is_unaired"]
        if value ~= nil then
            R.validate(V.check_Schema133, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "isUnaired", R.scalar(value) }
        end
    end
    do
        local value = params["min_community_rating"]
        if value ~= nil then
            R.validate(V.check_Schema134, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "minCommunityRating", R.scalar(value) }
        end
    end
    do
        local value = params["min_critic_rating"]
        if value ~= nil then
            R.validate(V.check_Schema135, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "minCriticRating", R.scalar(value) }
        end
    end
    do
        local value = params["min_premiere_date"]
        if value ~= nil then
            R.validate(V.check_Schema136, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "minPremiereDate", R.scalar(value) }
        end
    end
    do
        local value = params["min_date_last_saved"]
        if value ~= nil then
            R.validate(V.check_Schema137, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "minDateLastSaved", R.scalar(value) }
        end
    end
    do
        local value = params["min_date_last_saved_for_user"]
        if value ~= nil then
            R.validate(V.check_Schema138, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "minDateLastSavedForUser", R.scalar(value) }
        end
    end
    do
        local value = params["max_premiere_date"]
        if value ~= nil then
            R.validate(V.check_Schema139, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "maxPremiereDate", R.scalar(value) }
        end
    end
    do
        local value = params["has_overview"]
        if value ~= nil then
            R.validate(V.check_Schema140, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "hasOverview", R.scalar(value) }
        end
    end
    do
        local value = params["has_imdb_id"]
        if value ~= nil then
            R.validate(V.check_Schema141, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "hasImdbId", R.scalar(value) }
        end
    end
    do
        local value = params["has_tmdb_id"]
        if value ~= nil then
            R.validate(V.check_Schema142, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "hasTmdbId", R.scalar(value) }
        end
    end
    do
        local value = params["has_tvdb_id"]
        if value ~= nil then
            R.validate(V.check_Schema143, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "hasTvdbId", R.scalar(value) }
        end
    end
    do
        local value = params["is_movie"]
        if value ~= nil then
            R.validate(V.check_Schema144, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "isMovie", R.scalar(value) }
        end
    end
    do
        local value = params["is_series"]
        if value ~= nil then
            R.validate(V.check_Schema145, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "isSeries", R.scalar(value) }
        end
    end
    do
        local value = params["is_news"]
        if value ~= nil then
            R.validate(V.check_Schema146, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "isNews", R.scalar(value) }
        end
    end
    do
        local value = params["is_kids"]
        if value ~= nil then
            R.validate(V.check_Schema147, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "isKids", R.scalar(value) }
        end
    end
    do
        local value = params["is_sports"]
        if value ~= nil then
            R.validate(V.check_Schema148, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "isSports", R.scalar(value) }
        end
    end
    do
        local value = params["exclude_item_ids"]
        if value ~= nil then
            R.validate(V.check_Schema149, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "excludeItemIds", R.scalar(element) }
            end
        end
    end
    do
        local value = params["start_index"]
        if value ~= nil then
            R.validate(V.check_Schema150, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "startIndex", R.scalar(value) }
        end
    end
    do
        local value = params["limit"]
        if value ~= nil then
            R.validate(V.check_Schema151, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "limit", R.scalar(value) }
        end
    end
    do
        local value = params["recursive"]
        if value ~= nil then
            R.validate(V.check_Schema152, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "recursive", R.scalar(value) }
        end
    end
    do
        local value = params["search_term"]
        if value ~= nil then
            R.validate(V.check_Schema153, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "searchTerm", R.scalar(value) }
        end
    end
    do
        local value = params["sort_order"]
        if value ~= nil then
            R.validate(V.check_Schema154, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "sortOrder", R.scalar(element) }
            end
        end
    end
    do
        local value = params["parent_id"]
        if value ~= nil then
            R.validate(V.check_Schema155, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "parentId", R.scalar(value) }
        end
    end
    do
        local value = params["fields"]
        if value ~= nil then
            R.validate(V.check_Schema156, J, value)
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
            R.validate(V.check_Schema157, J, value)
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
            R.validate(V.check_Schema158, J, value)
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
            R.validate(V.check_Schema159, J, value)
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
            R.validate(V.check_Schema160, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "isFavorite", R.scalar(value) }
        end
    end
    do
        local value = params["media_types"]
        if value ~= nil then
            R.validate(V.check_Schema161, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "mediaTypes", R.scalar(element) }
            end
        end
    end
    do
        local value = params["image_types"]
        if value ~= nil then
            R.validate(V.check_Schema162, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "imageTypes", R.scalar(element) }
            end
        end
    end
    do
        local value = params["sort_by"]
        if value ~= nil then
            R.validate(V.check_Schema163, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "sortBy", R.scalar(element) }
            end
        end
    end
    do
        local value = params["is_played"]
        if value ~= nil then
            R.validate(V.check_Schema164, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "isPlayed", R.scalar(value) }
        end
    end
    do
        local value = params["genres"]
        if value ~= nil then
            R.validate(V.check_Schema165, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "genres", R.scalar(element) }
            end
        end
    end
    do
        local value = params["official_ratings"]
        if value ~= nil then
            R.validate(V.check_Schema166, J, value)
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
            R.validate(V.check_Schema167, J, value)
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
            R.validate(V.check_Schema168, J, value)
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
            R.validate(V.check_Schema169, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "enableUserData", R.scalar(value) }
        end
    end
    do
        local value = params["image_type_limit"]
        if value ~= nil then
            R.validate(V.check_Schema170, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "imageTypeLimit", R.scalar(value) }
        end
    end
    do
        local value = params["enable_image_types"]
        if value ~= nil then
            R.validate(V.check_Schema171, J, value)
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
            R.validate(V.check_Schema172, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "person", R.scalar(value) }
        end
    end
    do
        local value = params["person_ids"]
        if value ~= nil then
            R.validate(V.check_Schema173, J, value)
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
            R.validate(V.check_Schema174, J, value)
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
            R.validate(V.check_Schema175, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "studios", R.scalar(element) }
            end
        end
    end
    do
        local value = params["artists"]
        if value ~= nil then
            R.validate(V.check_Schema176, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "artists", R.scalar(element) }
            end
        end
    end
    do
        local value = params["exclude_artist_ids"]
        if value ~= nil then
            R.validate(V.check_Schema177, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "excludeArtistIds", R.scalar(element) }
            end
        end
    end
    do
        local value = params["artist_ids"]
        if value ~= nil then
            R.validate(V.check_Schema178, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "artistIds", R.scalar(element) }
            end
        end
    end
    do
        local value = params["album_artist_ids"]
        if value ~= nil then
            R.validate(V.check_Schema179, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "albumArtistIds", R.scalar(element) }
            end
        end
    end
    do
        local value = params["contributing_artist_ids"]
        if value ~= nil then
            R.validate(V.check_Schema180, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "contributingArtistIds", R.scalar(element) }
            end
        end
    end
    do
        local value = params["albums"]
        if value ~= nil then
            R.validate(V.check_Schema181, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "albums", R.scalar(element) }
            end
        end
    end
    do
        local value = params["album_ids"]
        if value ~= nil then
            R.validate(V.check_Schema182, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "albumIds", R.scalar(element) }
            end
        end
    end
    do
        local value = params["ids"]
        if value ~= nil then
            R.validate(V.check_Schema183, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "ids", R.scalar(element) }
            end
        end
    end
    do
        local value = params["video_types"]
        if value ~= nil then
            R.validate(V.check_Schema184, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "videoTypes", R.scalar(element) }
            end
        end
    end
    do
        local value = params["min_official_rating"]
        if value ~= nil then
            R.validate(V.check_Schema185, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "minOfficialRating", R.scalar(value) }
        end
    end
    do
        local value = params["is_locked"]
        if value ~= nil then
            R.validate(V.check_Schema186, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "isLocked", R.scalar(value) }
        end
    end
    do
        local value = params["is_place_holder"]
        if value ~= nil then
            R.validate(V.check_Schema187, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "isPlaceHolder", R.scalar(value) }
        end
    end
    do
        local value = params["has_official_rating"]
        if value ~= nil then
            R.validate(V.check_Schema188, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "hasOfficialRating", R.scalar(value) }
        end
    end
    do
        local value = params["collapse_box_set_items"]
        if value ~= nil then
            R.validate(V.check_Schema189, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "collapseBoxSetItems", R.scalar(value) }
        end
    end
    do
        local value = params["min_width"]
        if value ~= nil then
            R.validate(V.check_Schema190, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "minWidth", R.scalar(value) }
        end
    end
    do
        local value = params["min_height"]
        if value ~= nil then
            R.validate(V.check_Schema191, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "minHeight", R.scalar(value) }
        end
    end
    do
        local value = params["max_width"]
        if value ~= nil then
            R.validate(V.check_Schema192, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "maxWidth", R.scalar(value) }
        end
    end
    do
        local value = params["max_height"]
        if value ~= nil then
            R.validate(V.check_Schema193, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "maxHeight", R.scalar(value) }
        end
    end
    do
        local value = params["is3d"]
        if value ~= nil then
            R.validate(V.check_Schema194, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "is3D", R.scalar(value) }
        end
    end
    do
        local value = params["series_status"]
        if value ~= nil then
            R.validate(V.check_Schema195, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "seriesStatus", R.scalar(element) }
            end
        end
    end
    do
        local value = params["name_starts_with_or_greater"]
        if value ~= nil then
            R.validate(V.check_Schema196, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "nameStartsWithOrGreater", R.scalar(value) }
        end
    end
    do
        local value = params["name_starts_with"]
        if value ~= nil then
            R.validate(V.check_Schema197, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "nameStartsWith", R.scalar(value) }
        end
    end
    do
        local value = params["name_less_than"]
        if value ~= nil then
            R.validate(V.check_Schema198, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "nameLessThan", R.scalar(value) }
        end
    end
    do
        local value = params["studio_ids"]
        if value ~= nil then
            R.validate(V.check_Schema199, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "studioIds", R.scalar(element) }
            end
        end
    end
    do
        local value = params["genre_ids"]
        if value ~= nil then
            R.validate(V.check_Schema200, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            local values = {}
            for _, element in ipairs(value) do
                query[#query + 1] = { "genreIds", R.scalar(element) }
            end
        end
    end
    do
        local value = params["enable_total_record_count"]
        if value ~= nil then
            R.validate(V.check_Schema201, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "enableTotalRecordCount", R.scalar(value) }
        end
    end
    do
        local value = params["enable_images"]
        if value ~= nil then
            R.validate(V.check_Schema202, J, value)
            R.parameter(value ~= J.null, 'null parameter is unsupported')
            query[#query + 1] = { "enableImages", R.scalar(value) }
        end
    end
    R.parameters(params, {["user_id"] = true, ["max_official_rating"] = true, ["has_theme_song"] = true, ["has_theme_video"] = true, ["has_subtitles"] = true, ["has_special_feature"] = true, ["has_trailer"] = true, ["adjacent_to"] = true, ["index_number"] = true, ["parent_index_number"] = true, ["has_parental_rating"] = true, ["is_hd"] = true, ["is4k"] = true, ["location_types"] = true, ["exclude_location_types"] = true, ["is_missing"] = true, ["is_unaired"] = true, ["min_community_rating"] = true, ["min_critic_rating"] = true, ["min_premiere_date"] = true, ["min_date_last_saved"] = true, ["min_date_last_saved_for_user"] = true, ["max_premiere_date"] = true, ["has_overview"] = true, ["has_imdb_id"] = true, ["has_tmdb_id"] = true, ["has_tvdb_id"] = true, ["is_movie"] = true, ["is_series"] = true, ["is_news"] = true, ["is_kids"] = true, ["is_sports"] = true, ["exclude_item_ids"] = true, ["start_index"] = true, ["limit"] = true, ["recursive"] = true, ["search_term"] = true, ["sort_order"] = true, ["parent_id"] = true, ["fields"] = true, ["exclude_item_types"] = true, ["include_item_types"] = true, ["filters"] = true, ["is_favorite"] = true, ["media_types"] = true, ["image_types"] = true, ["sort_by"] = true, ["is_played"] = true, ["genres"] = true, ["official_ratings"] = true, ["tags"] = true, ["years"] = true, ["enable_user_data"] = true, ["image_type_limit"] = true, ["enable_image_types"] = true, ["person"] = true, ["person_ids"] = true, ["person_types"] = true, ["studios"] = true, ["artists"] = true, ["exclude_artist_ids"] = true, ["artist_ids"] = true, ["album_artist_ids"] = true, ["contributing_artist_ids"] = true, ["albums"] = true, ["album_ids"] = true, ["ids"] = true, ["video_types"] = true, ["min_official_rating"] = true, ["is_locked"] = true, ["is_place_holder"] = true, ["has_official_rating"] = true, ["collapse_box_set_items"] = true, ["min_width"] = true, ["min_height"] = true, ["max_width"] = true, ["max_height"] = true, ["is3d"] = true, ["series_status"] = true, ["name_starts_with_or_greater"] = true, ["name_starts_with"] = true, ["name_less_than"] = true, ["studio_ids"] = true, ["genre_ids"] = true, ["enable_total_record_count"] = true, ["enable_images"] = true, })
    R.authorize(options, {{["CustomAuthentication"] = {}, }, }, "GetItems", query, headers)
    R.extend_headers(headers, request_context and request_context.headers or {})
    headers[#headers + 1] = { 'Accept', options.json_media_type or 'application/json' }
    local builder = options.transport:request("GET", options.url.append_api_path(options.base_url, path))
        :json_codec('strict'):query_pairs(query):header_pairs(headers)
    return builder:build()
end

---@param response MluaHttpResponse
---@return jellyfin_api.models.BaseItemDtoQueryResult|jellyfin_api.models.BaseItemDtoQueryResult|jellyfin_api.models.BaseItemDtoQueryResult
function get_items:parse_response(response)
    local status = response:status()
    local context = { operation_id = "GetItems", status = status, content_type = response:header_values('content-type')[1] }
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

return get_items
