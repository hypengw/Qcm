---@class LuaApiOptions
---@field json MluaStrictJson
---@field url table
---@field transport MluaHttpClient
---@field base_url string
---@field authorize? fun(requirements: table[], operation_id: string): {headers?: string[][], query?: string[][], form?: string[][]}
---@field json_media_type? string
---@field headers_timeout_ms? integer

---@class LuaApiRequestContext
---@field headers? string[][]

local M = {}

---@return any
function M.fail(code, message, context)
    context = context or {}
    context.domain, context.code, context.message = 'openapi', code, message
    error(context, 0)
end

function M.check(condition, message, path)
    if not condition then M.fail('schema', message, { path = path }) end
end

function M.parameter(condition, message)
    if not condition then M.fail('parameter', message) end
end

function M.options(options)
    assert(options.json and options.url and options.transport and options.base_url,
        'json, url, transport and base_url are required')
end

function M.child_path(path, key)
    return path .. '/' .. key:gsub('~', '~0'):gsub('/', '~1')
end

function M.visit(value, path, state, depth)
    state.nodes = state.nodes + 1
    if state.nodes > 200000 or depth > 64 then M.fail('limit', 'model budget exceeded', { path = path }) end
    M.check(value ~= nil, 'missing value', path)
end

function M.validate(check, json, value)
    return check(json, value, '', { nodes = 0 }, 0)
end

function M.snapshot(J, value, state, depth)
    state, depth = state or { nodes = 0, bytes = 0, active = {} }, depth or 0
    state.nodes = state.nodes + 1
    if type(value) == 'string' then state.bytes = state.bytes + #value end
    if state.nodes > 200000 or depth > 64 or state.bytes > 8 * 1024 * 1024 then
        M.fail('limit', 'request snapshot budget exceeded')
    end
    if type(value) ~= 'table' then
        M.parameter(value == J.null or type(value) == 'string' or type(value) == 'boolean' or type(value) == 'number', 'unsupported request value')
        return value
    end
    M.parameter(not state.active[value], 'cyclic request value')
    state.active[value] = true
    local out = {}
    for key, child in pairs(value) do
        M.parameter(type(key) == 'string' or math.type(key) == 'integer', 'invalid request key')
        M.snapshot(J, key, state, depth + 1)
        out[key] = M.snapshot(J, child, state, depth + 1)
    end
    state.active[value] = nil
    local kind = J.kind(value)
    if kind == 'array' then J.array(out) elseif kind == 'object' then J.object(out) end
    return out
end

function M.array(J, value, path)
    M.check(type(value) == 'table' and J.kind(value) ~= 'object', 'expected array', path)
    local count = 0
    for key in pairs(value) do
        M.check(math.type(key) == 'integer' and key >= 1 and key <= #value, 'expected dense array', path)
        count = count + 1
    end
    M.check(count == #value, 'expected dense array', path)
    J.array(value)
    return count
end

function M.object(J, value, path)
    M.check(type(value) == 'table' and J.kind(value) ~= 'array', 'expected object', path)
    local count = 0
    for key in pairs(value) do
        M.check(type(key) == 'string', 'expected string key', path)
        count = count + 1
    end
    J.object(value)
    return count
end

function M.unique(J, value, path)
    local seen = {}
    for _, child in ipairs(value) do
        local key = J.encode(child)
        M.check(not seen[key], 'array items must be unique', path)
        seen[key] = true
    end
end

function M.enum(J, value, candidates, path)
    for _, candidate in ipairs(candidates) do
        if J.encode(candidate) == J.encode(value) then return end
    end
    M.check(false, 'value is outside enum', path)
end

function M.scalar(value)
    local kind = type(value)
    M.parameter(kind == 'string' or kind == 'number' or kind == 'boolean', 'expected scalar')
    return tostring(value)
end

function M.sorted_keys(value)
    local keys = {}
    for key in pairs(value) do keys[#keys + 1] = key end
    table.sort(keys)
    return keys
end

function M.path_parameter(url, path, wire, value)
    local first, last = path:find('{' .. wire .. '}', 1, true)
    M.parameter(first ~= nil, 'path parameter has no placeholder')
    return path:sub(1, first - 1) .. url.encode_path_segment(M.scalar(value)) .. path:sub(last + 1)
end

function M.parameters(params, allowed)
    for key in pairs(params) do M.parameter(allowed[key], 'unknown parameter: ' .. tostring(key)) end
end

function M.extend_headers(headers, additional)
    for _, pair in ipairs(additional) do
        for _, existing in ipairs(headers) do
            M.parameter(existing[1]:lower() ~= pair[1]:lower(), 'request header conflict')
        end
        headers[#headers + 1] = pair
    end
end

function M.form(J, value)
    M.object(J, value, '')
    local out = {}
    for _, key in ipairs(M.sorted_keys(value)) do
        local child = value[key]
        if type(child) == 'table' then
            M.array(J, child, '/' .. key)
            for _, element in ipairs(child) do out[#out + 1] = { key, M.scalar(element) } end
        else
            out[#out + 1] = { key, M.scalar(child) }
        end
    end
    return out
end

function M.authorize(options, requirements, operation_id, query, headers, form)
    local auth = options.authorize and options.authorize(requirements, operation_id) or {}
    M.extend_headers(headers, auth.headers or {})
    for _, pair in ipairs(auth.query or {}) do
        for _, existing in ipairs(query) do M.parameter(existing[1] ~= pair[1], 'authentication query conflict') end
        for _, existing in ipairs(form or {}) do M.parameter(existing[1] ~= pair[1], 'authentication form conflict') end
        query[#query + 1] = pair
    end
    M.parameter(not auth.form or form ~= nil, 'form authentication requires a form request')
    for _, pair in ipairs(auth.form or {}) do
        for _, existing in ipairs(form) do M.parameter(existing[1] ~= pair[1], 'authentication form conflict') end
        for _, existing in ipairs(query) do M.parameter(existing[1] ~= pair[1], 'authentication query conflict') end
        form[#form + 1] = pair
    end
end

function M.media_matches(received, expected)
    if not received then return false end
    local base = received:lower():match('^%s*([^;%s]+)')
    if not base then return false end
    return received == expected or base == expected or expected:match('/%*$') and base:sub(1, #expected - 1) == expected:sub(1, -2)
end

function M.is_json(received)
    local base = received and received:lower():match('^%s*([^;%s]+)')
    return base and (base == 'application/json' or base:match('%+json$'))
end

---@return any
function M.http_error(response, context)
    -- A failed diagnostic read must not replace the original HTTP error.
    local ok, body = pcall(function() return response:text { max_bytes = 4096, timeout_ms = 5000 } end)
    context.body = ok and body or nil
    M.fail('http', 'unexpected HTTP status', context)
end

---@return any
function M.media_error(response, context)
    response:close()
    M.fail('media_type', 'unexpected Content-Type', context)
end

---@return any
function M.json_response(J, response, context, check)
    if not M.is_json(context.content_type) then return M.media_error(response, context) end
    local ok, value = pcall(function() return response:json { codec = 'strict' } end)
    if not ok then M.fail('decode', 'invalid JSON response', context) end
    if not check then return value end
    local valid, result = pcall(M.validate, check, J, value)
    if not valid then
        context.path = type(result) == 'table' and result.path or nil
        M.fail('schema', type(result) == 'table' and result.message or 'invalid response model', context)
    end
    return result
end

return M
