local Api = require('subsonic_api.client')
local R = require('subsonic_api.runtime')
local J = qcm.json.strict
local Client = {}
Client.__index = Client

function Client.new(base_url, method, valid)
    assert(method and method.type == 'Username', 'Username authentication is required')
    local self = setmetatable({form = false, extensions = {}, valid = valid, username = method.username}, Client)
    local function sdk(form)
        return Api.new {
            base_url = base_url, json = J, url = qcm.url, transport = qcm.get_http_client(),
            authorize = function()
                self:check()
                local salt = qcm.random.hex(12)
                local token = qcm.crypto.hex.encode_low(qcm.crypto.digest('md5', method.pw .. salt))
                return {[form and 'form' or 'query'] = {
                    {'u', method.username}, {'t', token}, {'s', salt},
                    {'v', '1.16.1'}, {'c', qcm.inner.app_name}, {'f', 'json'},
                }}
            end,
        }
    end
    self.get, self.post = sdk(false), sdk(true)
    return self
end

function Client:check()
    if not self.valid() then qcm.error {code = 'not_auth'} end
end

function Client.raise(err)
    if type(err) == 'table' then
        if err.domain == 'subsonic' then
            if err.code == 40 or err.code == 44 then qcm.error {code = 'not_auth', message = err.message} end
            if err.code == 70 then qcm.error {code = 'not_found'} end
            qcm.error {code = 'subsonic_' .. err.code, message = err.message}
        end
        qcm.error {code = err.code, status = err.status,
            message = (err.operation_id or '') .. ' ' .. (err.path or '') .. ': '
                .. (err.status and ('HTTP ' .. err.status .. ' ') or '') .. (err.message or 'request failed')}
    end
    error(err, 0)
end

function Client.unwrap(value, payload)
    local envelope = assert(value['subsonic-response'], 'missing Subsonic response')
    if envelope.status == 'failed' then
        local err = assert(envelope.error, 'missing Subsonic error')
        assert(math.type(err.code) == 'integer', 'invalid Subsonic error code')
        error({domain = 'subsonic', code = err.code, message = err.message or 'Subsonic request failed'}, 0)
    end
    assert(envelope.status == 'ok', 'invalid Subsonic response status')
    assert(envelope.error == nil or envelope.error == J.null, 'unexpected Subsonic error on success')
    if payload then
        assert(type(envelope[payload]) == 'table', 'missing Subsonic payload: ' .. payload)
        return envelope[payload]
    end
    return envelope
end

local function wire(params)
    local out = {}
    for key, value in pairs(params or {}) do
        out[(key:gsub('_([a-z])', string.upper))] = value
    end
    return out
end

function Client:operation(name, params, get_only)
    if self.form and not get_only then
        return self.post.operations['post_' .. name] {body = wire(params)}
    end
    return self.get.operations[name](params or {})
end

local function perform(self, name, params, payload, get_only)
    self:check()
    local op = self:operation(name, params, get_only)
    local response = qcm.get_http_client():send(op:build_request())
    self:check()
    local value = op:parse_response(response)
    self:check()
    return Client.unwrap(value, payload)
end

function Client:call(name, params, payload, get_only)
    local ok, result = pcall(perform, self, name, params, payload, get_only)
    if not ok then
        if type(result) == 'table' and not result.operation_id then result.operation_id = name end
        error(result, 0)
    end
    return result
end

function Client.unsupported(err)
    return type(err) == 'table' and (
        err.domain == 'subsonic' and err.code == 70
        or err.domain == 'openapi' and (err.status == 404 or err.status == 405 or err.status == 501))
end

function Client:discover()
    local ping = self:call('ping', {}, nil, true)
    self.open_subsonic = ping.openSubsonic == true
    self.extensions, self.form = {}, false
    if self.open_subsonic then
        local ok, result = pcall(self.call, self, 'get_open_subsonic_extensions', {}, 'openSubsonicExtensions', true)
        if not ok and not Client.unsupported(result) then error(result, 0) end
        if ok then
            for _, extension in ipairs(result) do
                assert(type(extension.name) == 'string' and type(extension.versions) == 'table', 'invalid extension')
                local versions = {}
                for _, version in ipairs(extension.versions) do versions[version] = true end
                self.extensions[extension.name] = versions
            end
            self.form = self.extensions.formPost and self.extensions.formPost[1] or false
        end
    end
    return ping
end

function Client:scan_status()
    local ok, result = pcall(self.call, self, 'get_scan_status', {}, 'scanStatus')
    if not ok then
        if Client.unsupported(result) then return nil end
        error(result, 0)
    end
    assert(result.scanning == false, 'server is scanning; retry synchronization later')
    return result
end

function Client:media(name, params, headers)
    self:check()
    local op = self:operation(name, params, true)
    local function validate(response)
        self:check()
        local status = response:status()
        if status == 304 or status == 416 then return end
        if status == 401 then response:close(); qcm.error {code = 'not_auth'} end
        if status == 404 then response:close(); qcm.error {code = 'not_found'} end
        local content_type = response:header_values('content-type')[1]
        if R.is_json(content_type) then
            local value = response:json {codec = 'strict', max_bytes = 65536, timeout_ms = 5000}
            self.get.models.SubsonicResponse(value)
            Client.unwrap(value)
            error('unexpected JSON media response', 0)
        end
        if R.media_matches(content_type, 'text/xml') or R.media_matches(content_type, 'application/xml') then
            response:close()
            error('Subsonic returned an XML media error', 0)
        end
        op:parse_response(response)
        self:check()
    end
    local pairs_list = {}
    for key, value in pairs(headers or {}) do pairs_list[#pairs_list + 1] = {key, value} end
    return op:build_request {headers = pairs_list}, function(response)
        local ok, err = pcall(validate, response)
        if not ok then Client.raise(err) end
    end
end

function Client:read_media(name, params, headers)
    local request, validate = self:media(name, params, headers)
    local response = qcm.get_http_client():send(request)
    validate(response)
    return response
end

return Client
