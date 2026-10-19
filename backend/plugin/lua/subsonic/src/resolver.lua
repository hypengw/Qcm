local map = require('mapping')
local M = {}
function M.library(ctx)
    for _, library in ipairs(ctx:libraries()) do
        if library.native_id == 'music' then return library.library_id end
    end
    error('Subsonic library has not been synchronized', 0)
end
function M.scope(library, kind)
    return {library_id = library or qcm.json.strict.null, item_type = qcm.enum.ItemType[kind]}
end
local groups = {Artist = 'artists', Album = 'albums', Song = 'songs'}
function M.write(ctx, library, kind, rows, run, observed)
    local models = {}
    for first = 1, #rows, 20 do
        local batch = map.new(library)
        for index = first, math.min(first + 19, #rows) do batch:add(kind, rows[index]) end
        local result = (run or ctx):sync_batch(batch.data)
        for _, model in ipairs(result[groups[kind]]) do models[#models + 1] = model end
        for dependency, keys in pairs(batch.keys) do
            if #keys > 0 then
                if run then run:observe_items(M.scope(library, dependency), keys) end
                if observed then
                    for _, key in ipairs(keys) do observed[dependency][key.native_id] = key end
                end
            end
        end
    end
    return models
end
return M
