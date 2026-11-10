require("subsonic_api.types")

local R = require("subsonic_api.runtime")
local V = {}

function V.check_Schema1(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema2(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema79(J, value["id"], R.child_path(path, "id"), state, depth + 1)
    R.check(value["id"] ~= nil, "missing required property: id", path)
    return value
end

function V.check_Schema3(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 1, 'number out of range', path) end
    if type(value) == 'number' then R.check(value <= 500, 'number out of range', path) end
    return value
end

function V.check_Schema4(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    return value
end

function V.check_Schema5(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema6(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema7(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema8(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema9(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["fromYear"] ~= nil then
        V.check_Schema80(J, value["fromYear"], R.child_path(path, "fromYear"), state, depth + 1)
    end
    if value["genre"] ~= nil then
        V.check_Schema81(J, value["genre"], R.child_path(path, "genre"), state, depth + 1)
    end
    if value["musicFolderId"] ~= nil then
        V.check_Schema82(J, value["musicFolderId"], R.child_path(path, "musicFolderId"), state, depth + 1)
    end
    if value["offset"] ~= nil then
        V.check_Schema83(J, value["offset"], R.child_path(path, "offset"), state, depth + 1)
    end
    if value["size"] ~= nil then
        V.check_Schema84(J, value["size"], R.child_path(path, "size"), state, depth + 1)
    end
    if value["toYear"] ~= nil then
        V.check_Schema85(J, value["toYear"], R.child_path(path, "toYear"), state, depth + 1)
    end
    V.check_AlbumListType(J, value["type"], R.child_path(path, "type"), state, depth + 1)
    R.check(value["type"] ~= nil, "missing required property: type", path)
    return value
end

function V.check_Schema10(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema11(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema86(J, value["id"], R.child_path(path, "id"), state, depth + 1)
    R.check(value["id"] ~= nil, "missing required property: id", path)
    return value
end

function V.check_Schema12(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema13(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["musicFolderId"] ~= nil then
        V.check_Schema87(J, value["musicFolderId"], R.child_path(path, "musicFolderId"), state, depth + 1)
    end
    return value
end

function V.check_Schema14(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema15(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema16(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema17(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema18(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema19(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    return value
end

function V.check_Schema20(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema21(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema22(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema23(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    return value
end

function V.check_Schema24(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema25(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema26(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["artist"] ~= nil then
        V.check_Schema88(J, value["artist"], R.child_path(path, "artist"), state, depth + 1)
    end
    if value["title"] ~= nil then
        V.check_Schema89(J, value["title"], R.child_path(path, "title"), state, depth + 1)
    end
    return value
end

function V.check_Schema27(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema28(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema29(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["enhanced"] ~= nil then
        V.check_Schema90(J, value["enhanced"], R.child_path(path, "enhanced"), state, depth + 1)
    end
    V.check_Schema91(J, value["id"], R.child_path(path, "id"), state, depth + 1)
    R.check(value["id"] ~= nil, "missing required property: id", path)
    return value
end

function V.check_Schema30(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    return value
end

function V.check_Schema31(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    return value
end

function V.check_Schema32(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema33(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema92(J, value["id"], R.child_path(path, "id"), state, depth + 1)
    R.check(value["id"] ~= nil, "missing required property: id", path)
    return value
end

function V.check_Schema34(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema35(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["username"] ~= nil then
        V.check_Schema93(J, value["username"], R.child_path(path, "username"), state, depth + 1)
    end
    return value
end

function V.check_Schema36(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    if type(value) == 'number' then R.check(value <= 500, 'number out of range', path) end
    return value
end

function V.check_Schema37(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema38(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema39(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema40(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema41(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["fromYear"] ~= nil then
        V.check_Schema94(J, value["fromYear"], R.child_path(path, "fromYear"), state, depth + 1)
    end
    if value["genre"] ~= nil then
        V.check_Schema95(J, value["genre"], R.child_path(path, "genre"), state, depth + 1)
    end
    if value["musicFolderId"] ~= nil then
        V.check_Schema96(J, value["musicFolderId"], R.child_path(path, "musicFolderId"), state, depth + 1)
    end
    if value["size"] ~= nil then
        V.check_Schema97(J, value["size"], R.child_path(path, "size"), state, depth + 1)
    end
    if value["toYear"] ~= nil then
        V.check_Schema98(J, value["toYear"], R.child_path(path, "toYear"), state, depth + 1)
    end
    return value
end

function V.check_Schema42(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    return value
end

function V.check_Schema43(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema44(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema99(J, value["id"], R.child_path(path, "id"), state, depth + 1)
    R.check(value["id"] ~= nil, "missing required property: id", path)
    return value
end

function V.check_Schema45(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema46(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    return value
end

function V.check_Schema47(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    return value
end

function V.check_Schema48(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema49(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    return value
end

function V.check_Schema50(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    return value
end

function V.check_Schema51(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    return value
end

function V.check_Schema52(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    return value
end

function V.check_Schema53(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    return value
end

function V.check_Schema54(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    return value
end

function V.check_Schema55(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema56(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["albumCount"] ~= nil then
        V.check_Schema100(J, value["albumCount"], R.child_path(path, "albumCount"), state, depth + 1)
    end
    if value["albumOffset"] ~= nil then
        V.check_Schema101(J, value["albumOffset"], R.child_path(path, "albumOffset"), state, depth + 1)
    end
    if value["artistCount"] ~= nil then
        V.check_Schema102(J, value["artistCount"], R.child_path(path, "artistCount"), state, depth + 1)
    end
    if value["artistOffset"] ~= nil then
        V.check_Schema103(J, value["artistOffset"], R.child_path(path, "artistOffset"), state, depth + 1)
    end
    if value["musicFolderId"] ~= nil then
        V.check_Schema104(J, value["musicFolderId"], R.child_path(path, "musicFolderId"), state, depth + 1)
    end
    V.check_Schema105(J, value["query"], R.child_path(path, "query"), state, depth + 1)
    if value["songCount"] ~= nil then
        V.check_Schema106(J, value["songCount"], R.child_path(path, "songCount"), state, depth + 1)
    end
    if value["songOffset"] ~= nil then
        V.check_Schema107(J, value["songOffset"], R.child_path(path, "songOffset"), state, depth + 1)
    end
    R.check(value["query"] ~= nil, "missing required property: query", path)
    return value
end

function V.check_Schema57(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema108(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema58(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema109(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema59(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema110(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema60(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["albumId"] ~= nil then
        V.check_Schema111(J, value["albumId"], R.child_path(path, "albumId"), state, depth + 1)
    end
    if value["artistId"] ~= nil then
        V.check_Schema112(J, value["artistId"], R.child_path(path, "artistId"), state, depth + 1)
    end
    if value["id"] ~= nil then
        V.check_Schema113(J, value["id"], R.child_path(path, "id"), state, depth + 1)
    end
    return value
end

function V.check_Schema61(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema62(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    return value
end

function V.check_Schema63(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema64(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    return value
end

function V.check_Schema65(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema66(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema67(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema68(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema69(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema70(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    return value
end

function V.check_Schema71(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema72(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema73(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema74(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    return value
end

function V.check_Schema75(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema114(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema76(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema115(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema77(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema116(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema78(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["albumId"] ~= nil then
        V.check_Schema117(J, value["albumId"], R.child_path(path, "albumId"), state, depth + 1)
    end
    if value["artistId"] ~= nil then
        V.check_Schema118(J, value["artistId"], R.child_path(path, "artistId"), state, depth + 1)
    end
    if value["id"] ~= nil then
        V.check_Schema119(J, value["id"], R.child_path(path, "id"), state, depth + 1)
    end
    return value
end

function V.check_Schema79(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema80(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema81(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema82(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema83(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    return value
end

function V.check_Schema84(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 1, 'number out of range', path) end
    if type(value) == 'number' then R.check(value <= 500, 'number out of range', path) end
    return value
end

function V.check_Schema85(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema86(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema87(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema88(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema89(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema90(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema91(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema92(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema93(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema94(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema95(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema96(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema97(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    if type(value) == 'number' then R.check(value <= 500, 'number out of range', path) end
    return value
end

function V.check_Schema98(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema99(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema100(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    return value
end

function V.check_Schema101(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    return value
end

function V.check_Schema102(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    return value
end

function V.check_Schema103(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    return value
end

function V.check_Schema104(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema105(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema106(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    return value
end

function V.check_Schema107(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    return value
end

function V.check_Schema108(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema109(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema110(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema111(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema120(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema112(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema121(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema113(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema122(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema114(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema115(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema116(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema117(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    return value
end

function V.check_Schema118(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    return value
end

function V.check_Schema119(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    return value
end

function V.check_Schema120(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema121(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema122(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_GetAlbumResponse(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema124(J, value["subsonic-response"], R.child_path(path, "subsonic-response"), state, depth + 1)
    R.check(value["subsonic-response"] ~= nil, "missing required property: subsonic-response", path)
    return value
end

function V.check_Schema124(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["album"] ~= nil then
        V.check_AlbumID3WithSongs(J, value["album"], R.child_path(path, "album"), state, depth + 1)
    end
    if value["error"] ~= nil then
        V.check_SubsonicError(J, value["error"], R.child_path(path, "error"), state, depth + 1)
    end
    if value["openSubsonic"] ~= nil then
        V.check_Schema125(J, value["openSubsonic"], R.child_path(path, "openSubsonic"), state, depth + 1)
    end
    if value["serverVersion"] ~= nil then
        V.check_Schema126(J, value["serverVersion"], R.child_path(path, "serverVersion"), state, depth + 1)
    end
    V.check_Schema127(J, value["status"], R.child_path(path, "status"), state, depth + 1)
    if value["type"] ~= nil then
        V.check_Schema128(J, value["type"], R.child_path(path, "type"), state, depth + 1)
    end
    if value["version"] ~= nil then
        V.check_Schema129(J, value["version"], R.child_path(path, "version"), state, depth + 1)
    end
    R.check(value["status"] ~= nil, "missing required property: status", path)
    return value
end

function V.check_Schema125(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema126(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema127(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"ok", "failed", }), path)
    return value
end

function V.check_Schema128(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema129(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_AlbumListType(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"random", "newest", "highest", "frequent", "recent", "alphabeticalByName", "alphabeticalByArtist", "starred", "byYear", "byGenre", }), path)
    return value
end

function V.check_GetAlbumList2Response(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema132(J, value["subsonic-response"], R.child_path(path, "subsonic-response"), state, depth + 1)
    R.check(value["subsonic-response"] ~= nil, "missing required property: subsonic-response", path)
    return value
end

function V.check_Schema132(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["albumList2"] ~= nil then
        V.check_AlbumList2(J, value["albumList2"], R.child_path(path, "albumList2"), state, depth + 1)
    end
    if value["error"] ~= nil then
        V.check_SubsonicError(J, value["error"], R.child_path(path, "error"), state, depth + 1)
    end
    if value["openSubsonic"] ~= nil then
        V.check_Schema133(J, value["openSubsonic"], R.child_path(path, "openSubsonic"), state, depth + 1)
    end
    if value["serverVersion"] ~= nil then
        V.check_Schema134(J, value["serverVersion"], R.child_path(path, "serverVersion"), state, depth + 1)
    end
    V.check_Schema135(J, value["status"], R.child_path(path, "status"), state, depth + 1)
    if value["type"] ~= nil then
        V.check_Schema136(J, value["type"], R.child_path(path, "type"), state, depth + 1)
    end
    if value["version"] ~= nil then
        V.check_Schema137(J, value["version"], R.child_path(path, "version"), state, depth + 1)
    end
    R.check(value["status"] ~= nil, "missing required property: status", path)
    return value
end

function V.check_Schema133(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema134(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema135(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"ok", "failed", }), path)
    return value
end

function V.check_Schema136(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema137(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_GetArtistResponse(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema139(J, value["subsonic-response"], R.child_path(path, "subsonic-response"), state, depth + 1)
    R.check(value["subsonic-response"] ~= nil, "missing required property: subsonic-response", path)
    return value
end

function V.check_Schema139(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["artist"] ~= nil then
        V.check_ArtistWithAlbumsID3(J, value["artist"], R.child_path(path, "artist"), state, depth + 1)
    end
    if value["error"] ~= nil then
        V.check_SubsonicError(J, value["error"], R.child_path(path, "error"), state, depth + 1)
    end
    if value["openSubsonic"] ~= nil then
        V.check_Schema140(J, value["openSubsonic"], R.child_path(path, "openSubsonic"), state, depth + 1)
    end
    if value["serverVersion"] ~= nil then
        V.check_Schema141(J, value["serverVersion"], R.child_path(path, "serverVersion"), state, depth + 1)
    end
    V.check_Schema142(J, value["status"], R.child_path(path, "status"), state, depth + 1)
    if value["type"] ~= nil then
        V.check_Schema143(J, value["type"], R.child_path(path, "type"), state, depth + 1)
    end
    if value["version"] ~= nil then
        V.check_Schema144(J, value["version"], R.child_path(path, "version"), state, depth + 1)
    end
    R.check(value["status"] ~= nil, "missing required property: status", path)
    return value
end

function V.check_Schema140(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema141(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema142(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"ok", "failed", }), path)
    return value
end

function V.check_Schema143(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema144(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_GetArtistsResponse(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema146(J, value["subsonic-response"], R.child_path(path, "subsonic-response"), state, depth + 1)
    R.check(value["subsonic-response"] ~= nil, "missing required property: subsonic-response", path)
    return value
end

function V.check_Schema146(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["artists"] ~= nil then
        V.check_ArtistsID3(J, value["artists"], R.child_path(path, "artists"), state, depth + 1)
    end
    if value["error"] ~= nil then
        V.check_SubsonicError(J, value["error"], R.child_path(path, "error"), state, depth + 1)
    end
    if value["openSubsonic"] ~= nil then
        V.check_Schema147(J, value["openSubsonic"], R.child_path(path, "openSubsonic"), state, depth + 1)
    end
    if value["serverVersion"] ~= nil then
        V.check_Schema148(J, value["serverVersion"], R.child_path(path, "serverVersion"), state, depth + 1)
    end
    V.check_Schema149(J, value["status"], R.child_path(path, "status"), state, depth + 1)
    if value["type"] ~= nil then
        V.check_Schema150(J, value["type"], R.child_path(path, "type"), state, depth + 1)
    end
    if value["version"] ~= nil then
        V.check_Schema151(J, value["version"], R.child_path(path, "version"), state, depth + 1)
    end
    R.check(value["status"] ~= nil, "missing required property: status", path)
    return value
end

function V.check_Schema147(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema148(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema149(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"ok", "failed", }), path)
    return value
end

function V.check_Schema150(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema151(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_SubsonicResponse(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema153(J, value["subsonic-response"], R.child_path(path, "subsonic-response"), state, depth + 1)
    R.check(value["subsonic-response"] ~= nil, "missing required property: subsonic-response", path)
    return value
end

function V.check_Schema153(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["error"] ~= nil then
        V.check_SubsonicError(J, value["error"], R.child_path(path, "error"), state, depth + 1)
    end
    if value["openSubsonic"] ~= nil then
        V.check_Schema154(J, value["openSubsonic"], R.child_path(path, "openSubsonic"), state, depth + 1)
    end
    if value["serverVersion"] ~= nil then
        V.check_Schema155(J, value["serverVersion"], R.child_path(path, "serverVersion"), state, depth + 1)
    end
    V.check_Schema156(J, value["status"], R.child_path(path, "status"), state, depth + 1)
    if value["type"] ~= nil then
        V.check_Schema157(J, value["type"], R.child_path(path, "type"), state, depth + 1)
    end
    if value["version"] ~= nil then
        V.check_Schema158(J, value["version"], R.child_path(path, "version"), state, depth + 1)
    end
    R.check(value["status"] ~= nil, "missing required property: status", path)
    return value
end

function V.check_Schema154(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema155(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema156(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"ok", "failed", }), path)
    return value
end

function V.check_Schema157(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema158(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_GetLyricsResponse(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema160(J, value["subsonic-response"], R.child_path(path, "subsonic-response"), state, depth + 1)
    R.check(value["subsonic-response"] ~= nil, "missing required property: subsonic-response", path)
    return value
end

function V.check_Schema160(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["error"] ~= nil then
        V.check_SubsonicError(J, value["error"], R.child_path(path, "error"), state, depth + 1)
    end
    if value["lyrics"] ~= nil then
        V.check_Lyrics(J, value["lyrics"], R.child_path(path, "lyrics"), state, depth + 1)
    end
    if value["openSubsonic"] ~= nil then
        V.check_Schema161(J, value["openSubsonic"], R.child_path(path, "openSubsonic"), state, depth + 1)
    end
    if value["serverVersion"] ~= nil then
        V.check_Schema162(J, value["serverVersion"], R.child_path(path, "serverVersion"), state, depth + 1)
    end
    V.check_Schema163(J, value["status"], R.child_path(path, "status"), state, depth + 1)
    if value["type"] ~= nil then
        V.check_Schema164(J, value["type"], R.child_path(path, "type"), state, depth + 1)
    end
    if value["version"] ~= nil then
        V.check_Schema165(J, value["version"], R.child_path(path, "version"), state, depth + 1)
    end
    R.check(value["status"] ~= nil, "missing required property: status", path)
    return value
end

function V.check_Schema161(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema162(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema163(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"ok", "failed", }), path)
    return value
end

function V.check_Schema164(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema165(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_GetLyricsBySongIdResponse(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema167(J, value["subsonic-response"], R.child_path(path, "subsonic-response"), state, depth + 1)
    R.check(value["subsonic-response"] ~= nil, "missing required property: subsonic-response", path)
    return value
end

function V.check_Schema167(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["error"] ~= nil then
        V.check_SubsonicError(J, value["error"], R.child_path(path, "error"), state, depth + 1)
    end
    if value["lyricsList"] ~= nil then
        V.check_LyricsList(J, value["lyricsList"], R.child_path(path, "lyricsList"), state, depth + 1)
    end
    if value["openSubsonic"] ~= nil then
        V.check_Schema168(J, value["openSubsonic"], R.child_path(path, "openSubsonic"), state, depth + 1)
    end
    if value["serverVersion"] ~= nil then
        V.check_Schema169(J, value["serverVersion"], R.child_path(path, "serverVersion"), state, depth + 1)
    end
    V.check_Schema170(J, value["status"], R.child_path(path, "status"), state, depth + 1)
    if value["type"] ~= nil then
        V.check_Schema171(J, value["type"], R.child_path(path, "type"), state, depth + 1)
    end
    if value["version"] ~= nil then
        V.check_Schema172(J, value["version"], R.child_path(path, "version"), state, depth + 1)
    end
    R.check(value["status"] ~= nil, "missing required property: status", path)
    return value
end

function V.check_Schema168(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema169(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema170(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"ok", "failed", }), path)
    return value
end

function V.check_Schema171(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema172(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_GetMusicFoldersResponse(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema174(J, value["subsonic-response"], R.child_path(path, "subsonic-response"), state, depth + 1)
    R.check(value["subsonic-response"] ~= nil, "missing required property: subsonic-response", path)
    return value
end

function V.check_Schema174(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["error"] ~= nil then
        V.check_SubsonicError(J, value["error"], R.child_path(path, "error"), state, depth + 1)
    end
    if value["musicFolders"] ~= nil then
        V.check_MusicFolders(J, value["musicFolders"], R.child_path(path, "musicFolders"), state, depth + 1)
    end
    if value["openSubsonic"] ~= nil then
        V.check_Schema175(J, value["openSubsonic"], R.child_path(path, "openSubsonic"), state, depth + 1)
    end
    if value["serverVersion"] ~= nil then
        V.check_Schema176(J, value["serverVersion"], R.child_path(path, "serverVersion"), state, depth + 1)
    end
    V.check_Schema177(J, value["status"], R.child_path(path, "status"), state, depth + 1)
    if value["type"] ~= nil then
        V.check_Schema178(J, value["type"], R.child_path(path, "type"), state, depth + 1)
    end
    if value["version"] ~= nil then
        V.check_Schema179(J, value["version"], R.child_path(path, "version"), state, depth + 1)
    end
    R.check(value["status"] ~= nil, "missing required property: status", path)
    return value
end

function V.check_Schema175(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema176(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema177(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"ok", "failed", }), path)
    return value
end

function V.check_Schema178(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema179(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_GetOpenSubsonicExtensionsResponse(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema181(J, value["subsonic-response"], R.child_path(path, "subsonic-response"), state, depth + 1)
    R.check(value["subsonic-response"] ~= nil, "missing required property: subsonic-response", path)
    return value
end

function V.check_Schema181(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["error"] ~= nil then
        V.check_SubsonicError(J, value["error"], R.child_path(path, "error"), state, depth + 1)
    end
    if value["openSubsonic"] ~= nil then
        V.check_Schema182(J, value["openSubsonic"], R.child_path(path, "openSubsonic"), state, depth + 1)
    end
    if value["openSubsonicExtensions"] ~= nil then
        V.check_Schema183(J, value["openSubsonicExtensions"], R.child_path(path, "openSubsonicExtensions"), state, depth + 1)
    end
    if value["serverVersion"] ~= nil then
        V.check_Schema184(J, value["serverVersion"], R.child_path(path, "serverVersion"), state, depth + 1)
    end
    V.check_Schema185(J, value["status"], R.child_path(path, "status"), state, depth + 1)
    if value["type"] ~= nil then
        V.check_Schema186(J, value["type"], R.child_path(path, "type"), state, depth + 1)
    end
    if value["version"] ~= nil then
        V.check_Schema187(J, value["version"], R.child_path(path, "version"), state, depth + 1)
    end
    R.check(value["status"] ~= nil, "missing required property: status", path)
    return value
end

function V.check_Schema182(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema183(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_OpenSubsonicExtension(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema184(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema185(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"ok", "failed", }), path)
    return value
end

function V.check_Schema186(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema187(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_GetPlaylistResponse(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema189(J, value["subsonic-response"], R.child_path(path, "subsonic-response"), state, depth + 1)
    R.check(value["subsonic-response"] ~= nil, "missing required property: subsonic-response", path)
    return value
end

function V.check_Schema189(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["error"] ~= nil then
        V.check_SubsonicError(J, value["error"], R.child_path(path, "error"), state, depth + 1)
    end
    if value["openSubsonic"] ~= nil then
        V.check_Schema190(J, value["openSubsonic"], R.child_path(path, "openSubsonic"), state, depth + 1)
    end
    if value["playlist"] ~= nil then
        V.check_PlaylistWithSongs(J, value["playlist"], R.child_path(path, "playlist"), state, depth + 1)
    end
    if value["serverVersion"] ~= nil then
        V.check_Schema191(J, value["serverVersion"], R.child_path(path, "serverVersion"), state, depth + 1)
    end
    V.check_Schema192(J, value["status"], R.child_path(path, "status"), state, depth + 1)
    if value["type"] ~= nil then
        V.check_Schema193(J, value["type"], R.child_path(path, "type"), state, depth + 1)
    end
    if value["version"] ~= nil then
        V.check_Schema194(J, value["version"], R.child_path(path, "version"), state, depth + 1)
    end
    R.check(value["status"] ~= nil, "missing required property: status", path)
    return value
end

function V.check_Schema190(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema191(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema192(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"ok", "failed", }), path)
    return value
end

function V.check_Schema193(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema194(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_GetPlaylistsResponse(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema196(J, value["subsonic-response"], R.child_path(path, "subsonic-response"), state, depth + 1)
    R.check(value["subsonic-response"] ~= nil, "missing required property: subsonic-response", path)
    return value
end

function V.check_Schema196(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["error"] ~= nil then
        V.check_SubsonicError(J, value["error"], R.child_path(path, "error"), state, depth + 1)
    end
    if value["openSubsonic"] ~= nil then
        V.check_Schema197(J, value["openSubsonic"], R.child_path(path, "openSubsonic"), state, depth + 1)
    end
    if value["playlists"] ~= nil then
        V.check_Playlists(J, value["playlists"], R.child_path(path, "playlists"), state, depth + 1)
    end
    if value["serverVersion"] ~= nil then
        V.check_Schema198(J, value["serverVersion"], R.child_path(path, "serverVersion"), state, depth + 1)
    end
    V.check_Schema199(J, value["status"], R.child_path(path, "status"), state, depth + 1)
    if value["type"] ~= nil then
        V.check_Schema200(J, value["type"], R.child_path(path, "type"), state, depth + 1)
    end
    if value["version"] ~= nil then
        V.check_Schema201(J, value["version"], R.child_path(path, "version"), state, depth + 1)
    end
    R.check(value["status"] ~= nil, "missing required property: status", path)
    return value
end

function V.check_Schema197(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema198(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema199(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"ok", "failed", }), path)
    return value
end

function V.check_Schema200(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema201(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_GetRandomSongsResponse(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema203(J, value["subsonic-response"], R.child_path(path, "subsonic-response"), state, depth + 1)
    R.check(value["subsonic-response"] ~= nil, "missing required property: subsonic-response", path)
    return value
end

function V.check_Schema203(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["error"] ~= nil then
        V.check_SubsonicError(J, value["error"], R.child_path(path, "error"), state, depth + 1)
    end
    if value["openSubsonic"] ~= nil then
        V.check_Schema204(J, value["openSubsonic"], R.child_path(path, "openSubsonic"), state, depth + 1)
    end
    if value["randomSongs"] ~= nil then
        V.check_Songs(J, value["randomSongs"], R.child_path(path, "randomSongs"), state, depth + 1)
    end
    if value["serverVersion"] ~= nil then
        V.check_Schema205(J, value["serverVersion"], R.child_path(path, "serverVersion"), state, depth + 1)
    end
    V.check_Schema206(J, value["status"], R.child_path(path, "status"), state, depth + 1)
    if value["type"] ~= nil then
        V.check_Schema207(J, value["type"], R.child_path(path, "type"), state, depth + 1)
    end
    if value["version"] ~= nil then
        V.check_Schema208(J, value["version"], R.child_path(path, "version"), state, depth + 1)
    end
    R.check(value["status"] ~= nil, "missing required property: status", path)
    return value
end

function V.check_Schema204(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema205(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema206(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"ok", "failed", }), path)
    return value
end

function V.check_Schema207(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema208(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_GetScanStatusResponse(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema210(J, value["subsonic-response"], R.child_path(path, "subsonic-response"), state, depth + 1)
    R.check(value["subsonic-response"] ~= nil, "missing required property: subsonic-response", path)
    return value
end

function V.check_Schema210(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["error"] ~= nil then
        V.check_SubsonicError(J, value["error"], R.child_path(path, "error"), state, depth + 1)
    end
    if value["openSubsonic"] ~= nil then
        V.check_Schema211(J, value["openSubsonic"], R.child_path(path, "openSubsonic"), state, depth + 1)
    end
    if value["scanStatus"] ~= nil then
        V.check_ScanStatus(J, value["scanStatus"], R.child_path(path, "scanStatus"), state, depth + 1)
    end
    if value["serverVersion"] ~= nil then
        V.check_Schema212(J, value["serverVersion"], R.child_path(path, "serverVersion"), state, depth + 1)
    end
    V.check_Schema213(J, value["status"], R.child_path(path, "status"), state, depth + 1)
    if value["type"] ~= nil then
        V.check_Schema214(J, value["type"], R.child_path(path, "type"), state, depth + 1)
    end
    if value["version"] ~= nil then
        V.check_Schema215(J, value["version"], R.child_path(path, "version"), state, depth + 1)
    end
    R.check(value["status"] ~= nil, "missing required property: status", path)
    return value
end

function V.check_Schema211(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema212(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema213(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"ok", "failed", }), path)
    return value
end

function V.check_Schema214(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema215(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_GetSongResponse(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema217(J, value["subsonic-response"], R.child_path(path, "subsonic-response"), state, depth + 1)
    R.check(value["subsonic-response"] ~= nil, "missing required property: subsonic-response", path)
    return value
end

function V.check_Schema217(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["error"] ~= nil then
        V.check_SubsonicError(J, value["error"], R.child_path(path, "error"), state, depth + 1)
    end
    if value["openSubsonic"] ~= nil then
        V.check_Schema218(J, value["openSubsonic"], R.child_path(path, "openSubsonic"), state, depth + 1)
    end
    if value["serverVersion"] ~= nil then
        V.check_Schema219(J, value["serverVersion"], R.child_path(path, "serverVersion"), state, depth + 1)
    end
    if value["song"] ~= nil then
        V.check_Child(J, value["song"], R.child_path(path, "song"), state, depth + 1)
    end
    V.check_Schema220(J, value["status"], R.child_path(path, "status"), state, depth + 1)
    if value["type"] ~= nil then
        V.check_Schema221(J, value["type"], R.child_path(path, "type"), state, depth + 1)
    end
    if value["version"] ~= nil then
        V.check_Schema222(J, value["version"], R.child_path(path, "version"), state, depth + 1)
    end
    R.check(value["status"] ~= nil, "missing required property: status", path)
    return value
end

function V.check_Schema218(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema219(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema220(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"ok", "failed", }), path)
    return value
end

function V.check_Schema221(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema222(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_GetStarred2Response(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema224(J, value["subsonic-response"], R.child_path(path, "subsonic-response"), state, depth + 1)
    R.check(value["subsonic-response"] ~= nil, "missing required property: subsonic-response", path)
    return value
end

function V.check_Schema224(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["error"] ~= nil then
        V.check_SubsonicError(J, value["error"], R.child_path(path, "error"), state, depth + 1)
    end
    if value["openSubsonic"] ~= nil then
        V.check_Schema225(J, value["openSubsonic"], R.child_path(path, "openSubsonic"), state, depth + 1)
    end
    if value["serverVersion"] ~= nil then
        V.check_Schema226(J, value["serverVersion"], R.child_path(path, "serverVersion"), state, depth + 1)
    end
    if value["starred2"] ~= nil then
        V.check_Starred2(J, value["starred2"], R.child_path(path, "starred2"), state, depth + 1)
    end
    V.check_Schema227(J, value["status"], R.child_path(path, "status"), state, depth + 1)
    if value["type"] ~= nil then
        V.check_Schema228(J, value["type"], R.child_path(path, "type"), state, depth + 1)
    end
    if value["version"] ~= nil then
        V.check_Schema229(J, value["version"], R.child_path(path, "version"), state, depth + 1)
    end
    R.check(value["status"] ~= nil, "missing required property: status", path)
    return value
end

function V.check_Schema225(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema226(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema227(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"ok", "failed", }), path)
    return value
end

function V.check_Schema228(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema229(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Search3Response(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema231(J, value["subsonic-response"], R.child_path(path, "subsonic-response"), state, depth + 1)
    R.check(value["subsonic-response"] ~= nil, "missing required property: subsonic-response", path)
    return value
end

function V.check_Schema231(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["error"] ~= nil then
        V.check_SubsonicError(J, value["error"], R.child_path(path, "error"), state, depth + 1)
    end
    if value["openSubsonic"] ~= nil then
        V.check_Schema232(J, value["openSubsonic"], R.child_path(path, "openSubsonic"), state, depth + 1)
    end
    if value["searchResult3"] ~= nil then
        V.check_SearchResult3(J, value["searchResult3"], R.child_path(path, "searchResult3"), state, depth + 1)
    end
    if value["serverVersion"] ~= nil then
        V.check_Schema233(J, value["serverVersion"], R.child_path(path, "serverVersion"), state, depth + 1)
    end
    V.check_Schema234(J, value["status"], R.child_path(path, "status"), state, depth + 1)
    if value["type"] ~= nil then
        V.check_Schema235(J, value["type"], R.child_path(path, "type"), state, depth + 1)
    end
    if value["version"] ~= nil then
        V.check_Schema236(J, value["version"], R.child_path(path, "version"), state, depth + 1)
    end
    R.check(value["status"] ~= nil, "missing required property: status", path)
    return value
end

function V.check_Schema232(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema233(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema234(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"ok", "failed", }), path)
    return value
end

function V.check_Schema235(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema236(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_AlbumID3WithSongs(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    V.check_AlbumID3(J, value, path, state, depth + 1)
    V.check_Schema238(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema238(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema239(J, value["song"], R.child_path(path, "song"), state, depth + 1)
    R.check(value["song"] ~= nil, "missing required property: song", path)
    return value
end

function V.check_Schema239(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Child(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_SubsonicError(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema241(J, value["code"], R.child_path(path, "code"), state, depth + 1)
    if value["helpUrl"] ~= nil then
        V.check_Schema242(J, value["helpUrl"], R.child_path(path, "helpUrl"), state, depth + 1)
    end
    if value["message"] ~= nil then
        V.check_Schema243(J, value["message"], R.child_path(path, "message"), state, depth + 1)
    end
    R.check(value["code"] ~= nil, "missing required property: code", path)
    return value
end

function V.check_Schema241(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.enum(J, value, J.array({0, 10, 20, 30, 40, 41, 42, 43, 44, 50, 60, 70, }), path)
    return value
end

function V.check_Schema242(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema243(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_AlbumList2(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["album"] ~= nil then
        V.check_Schema245(J, value["album"], R.child_path(path, "album"), state, depth + 1)
    end
    return value
end

function V.check_Schema245(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_AlbumID3(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_ArtistWithAlbumsID3(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    V.check_ArtistID3(J, value, path, state, depth + 1)
    V.check_Schema247(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema247(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema248(J, value["album"], R.child_path(path, "album"), state, depth + 1)
    R.check(value["album"] ~= nil, "missing required property: album", path)
    return value
end

function V.check_Schema248(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_AlbumID3(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_ArtistsID3(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["ignoredArticles"] ~= nil then
        V.check_Schema250(J, value["ignoredArticles"], R.child_path(path, "ignoredArticles"), state, depth + 1)
    end
    if value["index"] ~= nil then
        V.check_Schema251(J, value["index"], R.child_path(path, "index"), state, depth + 1)
    end
    return value
end

function V.check_Schema250(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema251(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_IndexID3(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Lyrics(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["artist"] ~= nil then
        V.check_Schema253(J, value["artist"], R.child_path(path, "artist"), state, depth + 1)
    end
    if value["title"] ~= nil then
        V.check_Schema254(J, value["title"], R.child_path(path, "title"), state, depth + 1)
    end
    V.check_Schema255(J, value["value"], R.child_path(path, "value"), state, depth + 1)
    R.check(value["value"] ~= nil, "missing required property: value", path)
    return value
end

function V.check_Schema253(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema254(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema255(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_LyricsList(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["structuredLyrics"] ~= nil then
        V.check_Schema257(J, value["structuredLyrics"], R.child_path(path, "structuredLyrics"), state, depth + 1)
    end
    return value
end

function V.check_Schema257(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_StructuredLyrics(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_MusicFolders(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["musicFolder"] ~= nil then
        V.check_Schema259(J, value["musicFolder"], R.child_path(path, "musicFolder"), state, depth + 1)
    end
    return value
end

function V.check_Schema259(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_MusicFolder(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_OpenSubsonicExtension(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema261(J, value["name"], R.child_path(path, "name"), state, depth + 1)
    V.check_Schema262(J, value["versions"], R.child_path(path, "versions"), state, depth + 1)
    R.check(value["name"] ~= nil, "missing required property: name", path)
    R.check(value["versions"] ~= nil, "missing required property: versions", path)
    return value
end

function V.check_Schema261(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema262(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema263(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema263(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_PlaylistWithSongs(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    V.check_Playlist(J, value, path, state, depth + 1)
    V.check_Schema265(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema265(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["entry"] ~= nil then
        V.check_Schema266(J, value["entry"], R.child_path(path, "entry"), state, depth + 1)
    end
    return value
end

function V.check_Schema266(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Child(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Playlists(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["playlist"] ~= nil then
        V.check_Schema268(J, value["playlist"], R.child_path(path, "playlist"), state, depth + 1)
    end
    return value
end

function V.check_Schema268(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Playlist(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Songs(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["song"] ~= nil then
        V.check_Schema270(J, value["song"], R.child_path(path, "song"), state, depth + 1)
    end
    return value
end

function V.check_Schema270(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Child(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_ScanStatus(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["count"] ~= nil then
        V.check_Schema272(J, value["count"], R.child_path(path, "count"), state, depth + 1)
    end
    V.check_Schema273(J, value["scanning"], R.child_path(path, "scanning"), state, depth + 1)
    R.check(value["scanning"] ~= nil, "missing required property: scanning", path)
    return value
end

function V.check_Schema272(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema273(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Child(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["album"] ~= nil then
        V.check_Schema275(J, value["album"], R.child_path(path, "album"), state, depth + 1)
    end
    if value["albumArtists"] ~= nil then
        V.check_Schema276(J, value["albumArtists"], R.child_path(path, "albumArtists"), state, depth + 1)
    end
    if value["albumId"] ~= nil then
        V.check_Schema277(J, value["albumId"], R.child_path(path, "albumId"), state, depth + 1)
    end
    if value["artist"] ~= nil then
        V.check_Schema278(J, value["artist"], R.child_path(path, "artist"), state, depth + 1)
    end
    if value["artistId"] ~= nil then
        V.check_Schema279(J, value["artistId"], R.child_path(path, "artistId"), state, depth + 1)
    end
    if value["artists"] ~= nil then
        V.check_Schema280(J, value["artists"], R.child_path(path, "artists"), state, depth + 1)
    end
    if value["averageRating"] ~= nil then
        V.check_Schema281(J, value["averageRating"], R.child_path(path, "averageRating"), state, depth + 1)
    end
    if value["bitDepth"] ~= nil then
        V.check_Schema282(J, value["bitDepth"], R.child_path(path, "bitDepth"), state, depth + 1)
    end
    if value["bitRate"] ~= nil then
        V.check_Schema283(J, value["bitRate"], R.child_path(path, "bitRate"), state, depth + 1)
    end
    if value["bookmarkPosition"] ~= nil then
        V.check_Schema284(J, value["bookmarkPosition"], R.child_path(path, "bookmarkPosition"), state, depth + 1)
    end
    if value["bpm"] ~= nil then
        V.check_Schema285(J, value["bpm"], R.child_path(path, "bpm"), state, depth + 1)
    end
    if value["channelCount"] ~= nil then
        V.check_Schema286(J, value["channelCount"], R.child_path(path, "channelCount"), state, depth + 1)
    end
    if value["comment"] ~= nil then
        V.check_Schema287(J, value["comment"], R.child_path(path, "comment"), state, depth + 1)
    end
    if value["contentType"] ~= nil then
        V.check_Schema288(J, value["contentType"], R.child_path(path, "contentType"), state, depth + 1)
    end
    if value["contributors"] ~= nil then
        V.check_Schema289(J, value["contributors"], R.child_path(path, "contributors"), state, depth + 1)
    end
    if value["coverArt"] ~= nil then
        V.check_Schema290(J, value["coverArt"], R.child_path(path, "coverArt"), state, depth + 1)
    end
    if value["created"] ~= nil then
        V.check_Schema291(J, value["created"], R.child_path(path, "created"), state, depth + 1)
    end
    if value["discNumber"] ~= nil then
        V.check_Schema292(J, value["discNumber"], R.child_path(path, "discNumber"), state, depth + 1)
    end
    if value["displayAlbumArtist"] ~= nil then
        V.check_Schema293(J, value["displayAlbumArtist"], R.child_path(path, "displayAlbumArtist"), state, depth + 1)
    end
    if value["displayArtist"] ~= nil then
        V.check_Schema294(J, value["displayArtist"], R.child_path(path, "displayArtist"), state, depth + 1)
    end
    if value["displayComposer"] ~= nil then
        V.check_Schema295(J, value["displayComposer"], R.child_path(path, "displayComposer"), state, depth + 1)
    end
    if value["duration"] ~= nil then
        V.check_Schema296(J, value["duration"], R.child_path(path, "duration"), state, depth + 1)
    end
    if value["explicitStatus"] ~= nil then
        V.check_ExplicitStatus(J, value["explicitStatus"], R.child_path(path, "explicitStatus"), state, depth + 1)
    end
    if value["genre"] ~= nil then
        V.check_Schema297(J, value["genre"], R.child_path(path, "genre"), state, depth + 1)
    end
    if value["genres"] ~= nil then
        V.check_Schema298(J, value["genres"], R.child_path(path, "genres"), state, depth + 1)
    end
    if value["groupings"] ~= nil then
        V.check_Schema299(J, value["groupings"], R.child_path(path, "groupings"), state, depth + 1)
    end
    V.check_Schema300(J, value["id"], R.child_path(path, "id"), state, depth + 1)
    V.check_Schema301(J, value["isDir"], R.child_path(path, "isDir"), state, depth + 1)
    if value["isVideo"] ~= nil then
        V.check_Schema302(J, value["isVideo"], R.child_path(path, "isVideo"), state, depth + 1)
    end
    if value["isrc"] ~= nil then
        V.check_Schema303(J, value["isrc"], R.child_path(path, "isrc"), state, depth + 1)
    end
    if value["mediaType"] ~= nil then
        V.check_MediaType(J, value["mediaType"], R.child_path(path, "mediaType"), state, depth + 1)
    end
    if value["moods"] ~= nil then
        V.check_Schema304(J, value["moods"], R.child_path(path, "moods"), state, depth + 1)
    end
    if value["movements"] ~= nil then
        V.check_Schema305(J, value["movements"], R.child_path(path, "movements"), state, depth + 1)
    end
    if value["musicBrainzId"] ~= nil then
        V.check_Schema306(J, value["musicBrainzId"], R.child_path(path, "musicBrainzId"), state, depth + 1)
    end
    if value["originalHeight"] ~= nil then
        V.check_Schema307(J, value["originalHeight"], R.child_path(path, "originalHeight"), state, depth + 1)
    end
    if value["originalWidth"] ~= nil then
        V.check_Schema308(J, value["originalWidth"], R.child_path(path, "originalWidth"), state, depth + 1)
    end
    if value["parent"] ~= nil then
        V.check_Schema309(J, value["parent"], R.child_path(path, "parent"), state, depth + 1)
    end
    if value["path"] ~= nil then
        V.check_Schema310(J, value["path"], R.child_path(path, "path"), state, depth + 1)
    end
    if value["playCount"] ~= nil then
        V.check_Schema311(J, value["playCount"], R.child_path(path, "playCount"), state, depth + 1)
    end
    if value["played"] ~= nil then
        V.check_Schema312(J, value["played"], R.child_path(path, "played"), state, depth + 1)
    end
    if value["replayGain"] ~= nil then
        V.check_Schema313(J, value["replayGain"], R.child_path(path, "replayGain"), state, depth + 1)
    end
    if value["samplingRate"] ~= nil then
        V.check_Schema314(J, value["samplingRate"], R.child_path(path, "samplingRate"), state, depth + 1)
    end
    if value["size"] ~= nil then
        V.check_Schema315(J, value["size"], R.child_path(path, "size"), state, depth + 1)
    end
    if value["sortName"] ~= nil then
        V.check_Schema316(J, value["sortName"], R.child_path(path, "sortName"), state, depth + 1)
    end
    if value["starred"] ~= nil then
        V.check_Schema317(J, value["starred"], R.child_path(path, "starred"), state, depth + 1)
    end
    if value["suffix"] ~= nil then
        V.check_Schema318(J, value["suffix"], R.child_path(path, "suffix"), state, depth + 1)
    end
    V.check_Schema319(J, value["title"], R.child_path(path, "title"), state, depth + 1)
    if value["track"] ~= nil then
        V.check_Schema320(J, value["track"], R.child_path(path, "track"), state, depth + 1)
    end
    if value["transcodedContentType"] ~= nil then
        V.check_Schema321(J, value["transcodedContentType"], R.child_path(path, "transcodedContentType"), state, depth + 1)
    end
    if value["transcodedSuffix"] ~= nil then
        V.check_Schema322(J, value["transcodedSuffix"], R.child_path(path, "transcodedSuffix"), state, depth + 1)
    end
    if value["type"] ~= nil then
        V.check_GenericMediaType(J, value["type"], R.child_path(path, "type"), state, depth + 1)
    end
    if value["userRating"] ~= nil then
        V.check_Schema323(J, value["userRating"], R.child_path(path, "userRating"), state, depth + 1)
    end
    if value["works"] ~= nil then
        V.check_Schema324(J, value["works"], R.child_path(path, "works"), state, depth + 1)
    end
    if value["year"] ~= nil then
        V.check_Schema325(J, value["year"], R.child_path(path, "year"), state, depth + 1)
    end
    R.check(value["id"] ~= nil, "missing required property: id", path)
    R.check(value["isDir"] ~= nil, "missing required property: isDir", path)
    R.check(value["title"] ~= nil, "missing required property: title", path)
    return value
end

function V.check_Schema275(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema276(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ArtistID3(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema277(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema278(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema279(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema280(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ArtistID3(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema281(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    if type(value) == 'number' then R.check(value <= 5, 'number out of range', path) end
    return value
end

function V.check_Schema282(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema283(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema284(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema285(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema286(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema287(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema288(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema289(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Contributor(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema290(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema291(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema292(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema293(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema294(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema295(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema296(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema297(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema298(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ItemGenre(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema299(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema326(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema300(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema301(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema302(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema303(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema327(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema304(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema328(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema305(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema329(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema306(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema307(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema308(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema309(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema310(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema311(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema312(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema313(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    V.check_ReplayGain(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema314(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema315(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema316(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema317(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema318(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema319(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema320(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema321(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema322(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema323(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    if type(value) == 'number' then R.check(value <= 5, 'number out of range', path) end
    return value
end

function V.check_Schema324(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema330(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema325(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema326(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema327(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema328(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema329(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["count"] ~= nil then
        V.check_Schema331(J, value["count"], R.child_path(path, "count"), state, depth + 1)
    end
    V.check_Schema332(J, value["name"], R.child_path(path, "name"), state, depth + 1)
    if value["number"] ~= nil then
        V.check_Schema333(J, value["number"], R.child_path(path, "number"), state, depth + 1)
    end
    R.check(value["name"] ~= nil, "missing required property: name", path)
    return value
end

function V.check_Schema330(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["musicBrainzId"] ~= nil then
        V.check_Schema334(J, value["musicBrainzId"], R.child_path(path, "musicBrainzId"), state, depth + 1)
    end
    V.check_Schema335(J, value["name"], R.child_path(path, "name"), state, depth + 1)
    R.check(value["name"] ~= nil, "missing required property: name", path)
    return value
end

function V.check_Schema331(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema332(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema333(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema334(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema335(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Starred2(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["album"] ~= nil then
        V.check_Schema337(J, value["album"], R.child_path(path, "album"), state, depth + 1)
    end
    if value["artist"] ~= nil then
        V.check_Schema338(J, value["artist"], R.child_path(path, "artist"), state, depth + 1)
    end
    if value["song"] ~= nil then
        V.check_Schema339(J, value["song"], R.child_path(path, "song"), state, depth + 1)
    end
    return value
end

function V.check_Schema337(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_AlbumID3(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema338(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ArtistID3(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema339(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Child(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_SearchResult3(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["album"] ~= nil then
        V.check_Schema341(J, value["album"], R.child_path(path, "album"), state, depth + 1)
    end
    if value["artist"] ~= nil then
        V.check_Schema342(J, value["artist"], R.child_path(path, "artist"), state, depth + 1)
    end
    if value["song"] ~= nil then
        V.check_Schema343(J, value["song"], R.child_path(path, "song"), state, depth + 1)
    end
    return value
end

function V.check_Schema341(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_AlbumID3(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema342(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ArtistID3(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema343(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Child(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_AlbumID3(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["artist"] ~= nil then
        V.check_Schema345(J, value["artist"], R.child_path(path, "artist"), state, depth + 1)
    end
    if value["artistId"] ~= nil then
        V.check_Schema346(J, value["artistId"], R.child_path(path, "artistId"), state, depth + 1)
    end
    if value["artists"] ~= nil then
        V.check_Schema347(J, value["artists"], R.child_path(path, "artists"), state, depth + 1)
    end
    if value["coverArt"] ~= nil then
        V.check_Schema348(J, value["coverArt"], R.child_path(path, "coverArt"), state, depth + 1)
    end
    V.check_Schema349(J, value["created"], R.child_path(path, "created"), state, depth + 1)
    if value["discTitles"] ~= nil then
        V.check_Schema350(J, value["discTitles"], R.child_path(path, "discTitles"), state, depth + 1)
    end
    if value["displayArtist"] ~= nil then
        V.check_Schema351(J, value["displayArtist"], R.child_path(path, "displayArtist"), state, depth + 1)
    end
    V.check_Schema352(J, value["duration"], R.child_path(path, "duration"), state, depth + 1)
    if value["explicitStatus"] ~= nil then
        V.check_ExplicitStatus(J, value["explicitStatus"], R.child_path(path, "explicitStatus"), state, depth + 1)
    end
    if value["genre"] ~= nil then
        V.check_Schema353(J, value["genre"], R.child_path(path, "genre"), state, depth + 1)
    end
    if value["genres"] ~= nil then
        V.check_Schema354(J, value["genres"], R.child_path(path, "genres"), state, depth + 1)
    end
    V.check_Schema355(J, value["id"], R.child_path(path, "id"), state, depth + 1)
    if value["isCompilation"] ~= nil then
        V.check_Schema356(J, value["isCompilation"], R.child_path(path, "isCompilation"), state, depth + 1)
    end
    if value["moods"] ~= nil then
        V.check_Schema357(J, value["moods"], R.child_path(path, "moods"), state, depth + 1)
    end
    if value["musicBrainzId"] ~= nil then
        V.check_Schema358(J, value["musicBrainzId"], R.child_path(path, "musicBrainzId"), state, depth + 1)
    end
    V.check_Schema359(J, value["name"], R.child_path(path, "name"), state, depth + 1)
    if value["originalReleaseDate"] ~= nil then
        V.check_Schema360(J, value["originalReleaseDate"], R.child_path(path, "originalReleaseDate"), state, depth + 1)
    end
    if value["playCount"] ~= nil then
        V.check_Schema361(J, value["playCount"], R.child_path(path, "playCount"), state, depth + 1)
    end
    if value["played"] ~= nil then
        V.check_Schema362(J, value["played"], R.child_path(path, "played"), state, depth + 1)
    end
    if value["recordLabels"] ~= nil then
        V.check_Schema363(J, value["recordLabels"], R.child_path(path, "recordLabels"), state, depth + 1)
    end
    if value["releaseDate"] ~= nil then
        V.check_Schema364(J, value["releaseDate"], R.child_path(path, "releaseDate"), state, depth + 1)
    end
    if value["releaseTypes"] ~= nil then
        V.check_Schema365(J, value["releaseTypes"], R.child_path(path, "releaseTypes"), state, depth + 1)
    end
    if value["song"] ~= nil then
        V.check_Schema366(J, value["song"], R.child_path(path, "song"), state, depth + 1)
    end
    V.check_Schema367(J, value["songCount"], R.child_path(path, "songCount"), state, depth + 1)
    if value["sortName"] ~= nil then
        V.check_Schema368(J, value["sortName"], R.child_path(path, "sortName"), state, depth + 1)
    end
    if value["starred"] ~= nil then
        V.check_Schema369(J, value["starred"], R.child_path(path, "starred"), state, depth + 1)
    end
    if value["userRating"] ~= nil then
        V.check_Schema370(J, value["userRating"], R.child_path(path, "userRating"), state, depth + 1)
    end
    if value["version"] ~= nil then
        V.check_Schema371(J, value["version"], R.child_path(path, "version"), state, depth + 1)
    end
    if value["year"] ~= nil then
        V.check_Schema372(J, value["year"], R.child_path(path, "year"), state, depth + 1)
    end
    R.check(value["id"] ~= nil, "missing required property: id", path)
    R.check(value["name"] ~= nil, "missing required property: name", path)
    R.check(value["songCount"] ~= nil, "missing required property: songCount", path)
    R.check(value["duration"] ~= nil, "missing required property: duration", path)
    R.check(value["created"] ~= nil, "missing required property: created", path)
    return value
end

function V.check_Schema345(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema346(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema347(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ArtistID3(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema348(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema349(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema350(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_DiscTitle(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema351(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema352(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema353(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema354(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ItemGenre(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema355(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema356(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema357(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema373(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema358(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema359(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema360(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    V.check_ItemDate(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema361(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema362(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema363(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_RecordLabel(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema364(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    V.check_ItemDate(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema365(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema374(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema366(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Child(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema367(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema368(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema369(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema370(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    if type(value) == 'number' then R.check(value <= 5, 'number out of range', path) end
    return value
end

function V.check_Schema371(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema372(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema373(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema374(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_ArtistID3(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["albumCount"] ~= nil then
        V.check_Schema376(J, value["albumCount"], R.child_path(path, "albumCount"), state, depth + 1)
    end
    if value["artistImageUrl"] ~= nil then
        V.check_Schema377(J, value["artistImageUrl"], R.child_path(path, "artistImageUrl"), state, depth + 1)
    end
    if value["coverArt"] ~= nil then
        V.check_Schema378(J, value["coverArt"], R.child_path(path, "coverArt"), state, depth + 1)
    end
    if value["disambiguation"] ~= nil then
        V.check_Schema379(J, value["disambiguation"], R.child_path(path, "disambiguation"), state, depth + 1)
    end
    V.check_Schema380(J, value["id"], R.child_path(path, "id"), state, depth + 1)
    if value["musicBrainzId"] ~= nil then
        V.check_Schema381(J, value["musicBrainzId"], R.child_path(path, "musicBrainzId"), state, depth + 1)
    end
    V.check_Schema382(J, value["name"], R.child_path(path, "name"), state, depth + 1)
    if value["roles"] ~= nil then
        V.check_Schema383(J, value["roles"], R.child_path(path, "roles"), state, depth + 1)
    end
    if value["sortName"] ~= nil then
        V.check_Schema384(J, value["sortName"], R.child_path(path, "sortName"), state, depth + 1)
    end
    if value["starred"] ~= nil then
        V.check_Schema385(J, value["starred"], R.child_path(path, "starred"), state, depth + 1)
    end
    R.check(value["id"] ~= nil, "missing required property: id", path)
    R.check(value["name"] ~= nil, "missing required property: name", path)
    return value
end

function V.check_Schema376(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema377(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema378(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema379(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema380(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema381(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema382(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema383(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema386(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema384(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema385(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema386(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_IndexID3(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["artist"] ~= nil then
        V.check_Schema388(J, value["artist"], R.child_path(path, "artist"), state, depth + 1)
    end
    V.check_Schema389(J, value["name"], R.child_path(path, "name"), state, depth + 1)
    R.check(value["name"] ~= nil, "missing required property: name", path)
    return value
end

function V.check_Schema388(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ArtistID3(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema389(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_StructuredLyrics(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["agents"] ~= nil then
        V.check_Schema391(J, value["agents"], R.child_path(path, "agents"), state, depth + 1)
    end
    if value["cueLine"] ~= nil then
        V.check_Schema392(J, value["cueLine"], R.child_path(path, "cueLine"), state, depth + 1)
    end
    if value["displayArtist"] ~= nil then
        V.check_Schema393(J, value["displayArtist"], R.child_path(path, "displayArtist"), state, depth + 1)
    end
    if value["displayTitle"] ~= nil then
        V.check_Schema394(J, value["displayTitle"], R.child_path(path, "displayTitle"), state, depth + 1)
    end
    if value["kind"] ~= nil then
        V.check_Schema395(J, value["kind"], R.child_path(path, "kind"), state, depth + 1)
    end
    V.check_Schema396(J, value["lang"], R.child_path(path, "lang"), state, depth + 1)
    V.check_Schema397(J, value["line"], R.child_path(path, "line"), state, depth + 1)
    if value["offset"] ~= nil then
        V.check_Schema398(J, value["offset"], R.child_path(path, "offset"), state, depth + 1)
    end
    V.check_Schema399(J, value["synced"], R.child_path(path, "synced"), state, depth + 1)
    R.check(value["lang"] ~= nil, "missing required property: lang", path)
    R.check(value["synced"] ~= nil, "missing required property: synced", path)
    R.check(value["line"] ~= nil, "missing required property: line", path)
    return value
end

function V.check_Schema391(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Agent(J, child, path .. '/' .. index, state, depth + 1)
    end
    R.check(length >= 1, 'length out of range', path)
    return value
end

function V.check_Schema392(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_CueLine(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema393(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema394(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema395(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"main", "translation", "pronunciation", }), path)
    return value
end

function V.check_Schema396(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema397(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Line(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema398(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema399(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_MusicFolder(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema401(J, value["id"], R.child_path(path, "id"), state, depth + 1)
    if value["name"] ~= nil then
        V.check_Schema402(J, value["name"], R.child_path(path, "name"), state, depth + 1)
    end
    R.check(value["id"] ~= nil, "missing required property: id", path)
    return value
end

function V.check_Schema401(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema402(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Playlist(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["allowedUser"] ~= nil then
        V.check_Schema404(J, value["allowedUser"], R.child_path(path, "allowedUser"), state, depth + 1)
    end
    V.check_Schema405(J, value["changed"], R.child_path(path, "changed"), state, depth + 1)
    if value["comment"] ~= nil then
        V.check_Schema406(J, value["comment"], R.child_path(path, "comment"), state, depth + 1)
    end
    if value["coverArt"] ~= nil then
        V.check_Schema407(J, value["coverArt"], R.child_path(path, "coverArt"), state, depth + 1)
    end
    V.check_Schema408(J, value["created"], R.child_path(path, "created"), state, depth + 1)
    V.check_Schema409(J, value["duration"], R.child_path(path, "duration"), state, depth + 1)
    V.check_Schema410(J, value["id"], R.child_path(path, "id"), state, depth + 1)
    V.check_Schema411(J, value["name"], R.child_path(path, "name"), state, depth + 1)
    if value["owner"] ~= nil then
        V.check_Schema412(J, value["owner"], R.child_path(path, "owner"), state, depth + 1)
    end
    if value["public"] ~= nil then
        V.check_Schema413(J, value["public"], R.child_path(path, "public"), state, depth + 1)
    end
    if value["readonly"] ~= nil then
        V.check_Schema414(J, value["readonly"], R.child_path(path, "readonly"), state, depth + 1)
    end
    V.check_Schema415(J, value["songCount"], R.child_path(path, "songCount"), state, depth + 1)
    if value["validUntil"] ~= nil then
        V.check_Schema416(J, value["validUntil"], R.child_path(path, "validUntil"), state, depth + 1)
    end
    R.check(value["id"] ~= nil, "missing required property: id", path)
    R.check(value["name"] ~= nil, "missing required property: name", path)
    R.check(value["songCount"] ~= nil, "missing required property: songCount", path)
    R.check(value["duration"] ~= nil, "missing required property: duration", path)
    R.check(value["created"] ~= nil, "missing required property: created", path)
    R.check(value["changed"] ~= nil, "missing required property: changed", path)
    return value
end

function V.check_Schema404(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema417(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema405(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema406(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema407(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema408(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema409(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema410(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema411(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema412(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema413(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema414(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema415(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema416(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema417(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_ExplicitStatus(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"clean", "explicit", "", }), path)
    return value
end

function V.check_MediaType(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"song", "album", "artist", }), path)
    return value
end

function V.check_GenericMediaType(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"music", "video", "podcast", "audiobook", }), path)
    return value
end

function V.check_Contributor(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_ArtistID3(J, value["artist"], R.child_path(path, "artist"), state, depth + 1)
    V.check_Schema422(J, value["role"], R.child_path(path, "role"), state, depth + 1)
    if value["subRole"] ~= nil then
        V.check_Schema423(J, value["subRole"], R.child_path(path, "subRole"), state, depth + 1)
    end
    R.check(value["role"] ~= nil, "missing required property: role", path)
    R.check(value["artist"] ~= nil, "missing required property: artist", path)
    return value
end

function V.check_Schema422(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema423(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_ItemGenre(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema425(J, value["name"], R.child_path(path, "name"), state, depth + 1)
    R.check(value["name"] ~= nil, "missing required property: name", path)
    return value
end

function V.check_Schema425(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_ReplayGain(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["albumGain"] ~= nil then
        V.check_Schema427(J, value["albumGain"], R.child_path(path, "albumGain"), state, depth + 1)
    end
    if value["albumPeak"] ~= nil then
        V.check_Schema428(J, value["albumPeak"], R.child_path(path, "albumPeak"), state, depth + 1)
    end
    if value["baseGain"] ~= nil then
        V.check_Schema429(J, value["baseGain"], R.child_path(path, "baseGain"), state, depth + 1)
    end
    if value["fallbackGain"] ~= nil then
        V.check_Schema430(J, value["fallbackGain"], R.child_path(path, "fallbackGain"), state, depth + 1)
    end
    if value["trackGain"] ~= nil then
        V.check_Schema431(J, value["trackGain"], R.child_path(path, "trackGain"), state, depth + 1)
    end
    if value["trackPeak"] ~= nil then
        V.check_Schema432(J, value["trackPeak"], R.child_path(path, "trackPeak"), state, depth + 1)
    end
    return value
end

function V.check_Schema427(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema428(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    return value
end

function V.check_Schema429(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema430(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema431(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema432(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    return value
end

function V.check_DiscTitle(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["coverArt"] ~= nil then
        V.check_Schema434(J, value["coverArt"], R.child_path(path, "coverArt"), state, depth + 1)
    end
    V.check_Schema435(J, value["disc"], R.child_path(path, "disc"), state, depth + 1)
    V.check_Schema436(J, value["title"], R.child_path(path, "title"), state, depth + 1)
    R.check(value["disc"] ~= nil, "missing required property: disc", path)
    R.check(value["title"] ~= nil, "missing required property: title", path)
    return value
end

function V.check_Schema434(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema435(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema436(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_ItemDate(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["day"] ~= nil then
        V.check_Schema438(J, value["day"], R.child_path(path, "day"), state, depth + 1)
    end
    if value["month"] ~= nil then
        V.check_Schema439(J, value["month"], R.child_path(path, "month"), state, depth + 1)
    end
    if value["year"] ~= nil then
        V.check_Schema440(J, value["year"], R.child_path(path, "year"), state, depth + 1)
    end
    return value
end

function V.check_Schema438(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 1, 'number out of range', path) end
    if type(value) == 'number' then R.check(value <= 31, 'number out of range', path) end
    return value
end

function V.check_Schema439(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 1, 'number out of range', path) end
    if type(value) == 'number' then R.check(value <= 12, 'number out of range', path) end
    return value
end

function V.check_Schema440(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_RecordLabel(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema442(J, value["name"], R.child_path(path, "name"), state, depth + 1)
    R.check(value["name"] ~= nil, "missing required property: name", path)
    return value
end

function V.check_Schema442(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Agent(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema444(J, value["id"], R.child_path(path, "id"), state, depth + 1)
    if value["name"] ~= nil then
        V.check_Schema445(J, value["name"], R.child_path(path, "name"), state, depth + 1)
    end
    V.check_Schema446(J, value["role"], R.child_path(path, "role"), state, depth + 1)
    R.check(value["id"] ~= nil, "missing required property: id", path)
    R.check(value["role"] ~= nil, "missing required property: role", path)
    return value
end

function V.check_Schema444(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema445(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema446(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"main", "voice", "bg", "group", }), path)
    return value
end

function V.check_CueLine(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["agentId"] ~= nil then
        V.check_Schema448(J, value["agentId"], R.child_path(path, "agentId"), state, depth + 1)
    end
    V.check_Schema449(J, value["cue"], R.child_path(path, "cue"), state, depth + 1)
    if value["end"] ~= nil then
        V.check_Schema450(J, value["end"], R.child_path(path, "end"), state, depth + 1)
    end
    V.check_Schema451(J, value["index"], R.child_path(path, "index"), state, depth + 1)
    if value["start"] ~= nil then
        V.check_Schema452(J, value["start"], R.child_path(path, "start"), state, depth + 1)
    end
    V.check_Schema453(J, value["value"], R.child_path(path, "value"), state, depth + 1)
    R.check(value["index"] ~= nil, "missing required property: index", path)
    R.check(value["value"] ~= nil, "missing required property: value", path)
    R.check(value["cue"] ~= nil, "missing required property: cue", path)
    return value
end

function V.check_Schema448(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema449(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Cue(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema450(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema451(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema452(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema453(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Line(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["start"] ~= nil then
        V.check_Schema455(J, value["start"], R.child_path(path, "start"), state, depth + 1)
    end
    V.check_Schema456(J, value["value"], R.child_path(path, "value"), state, depth + 1)
    R.check(value["value"] ~= nil, "missing required property: value", path)
    return value
end

function V.check_Schema455(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema456(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Cue(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    V.check_Schema458(J, value["byteEnd"], R.child_path(path, "byteEnd"), state, depth + 1)
    V.check_Schema459(J, value["byteStart"], R.child_path(path, "byteStart"), state, depth + 1)
    if value["end"] ~= nil then
        V.check_Schema460(J, value["end"], R.child_path(path, "end"), state, depth + 1)
    end
    V.check_Schema461(J, value["start"], R.child_path(path, "start"), state, depth + 1)
    V.check_Schema462(J, value["value"], R.child_path(path, "value"), state, depth + 1)
    R.check(value["start"] ~= nil, "missing required property: start", path)
    R.check(value["byteStart"] ~= nil, "missing required property: byteStart", path)
    R.check(value["byteEnd"] ~= nil, "missing required property: byteEnd", path)
    R.check(value["value"] ~= nil, "missing required property: value", path)
    return value
end

function V.check_Schema458(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    return value
end

function V.check_Schema459(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    if type(value) == 'number' then R.check(value >= 0, 'number out of range', path) end
    return value
end

function V.check_Schema460(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema461(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema462(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

return V
