require("jellyfin_api.types")

local R = require("jellyfin_api.runtime")
local V = {}

function V.check_Schema1(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema2(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema3(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema4(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema5(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema6(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ItemFields(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema7(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_BaseItemKind(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema8(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_BaseItemKind(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema9(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ItemFilter(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema10(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema11(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_MediaType(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema12(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema246(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema13(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema247(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema14(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema248(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema15(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema249(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema16(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema250(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema17(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema18(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema19(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ImageType(J, child, path .. '/' .. index, state, depth + 1)
    end
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
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema251(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema22(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema252(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema23(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema253(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema24(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema254(J, child, path .. '/' .. index, state, depth + 1)
    end
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
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
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
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema29(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ItemSortBy(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema30(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_SortOrder(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema31(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema32(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema33(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema34(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema35(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema36(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
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
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ItemFields(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema39(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_BaseItemKind(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema40(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_BaseItemKind(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema41(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ItemFilter(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema42(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema43(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_MediaType(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema44(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema255(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema45(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema256(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema46(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema257(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema47(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema258(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema48(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema259(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema49(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema50(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema51(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ImageType(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema52(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema53(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema260(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema54(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema261(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema55(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema262(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema56(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema263(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema57(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema58(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema59(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema60(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema61(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ItemSortBy(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema62(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_SortOrder(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema63(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema64(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema65(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema66(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema67(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.check(#value >= 0 and #value <= 40, 'pattern length mismatch', path)
    for index = 1, #value do
        local byte = value:byte(index)
        R.check((byte >= 97 and byte <= 122) or (byte >= 65 and byte <= 90) or (byte >= 48 and byte <= 57) or (byte >= 45 and byte <= 45) or (byte >= 46 and byte <= 46) or (byte >= 95 and byte <= 95) or (byte >= 44 and byte <= 44) or (byte >= 124 and byte <= 124), 'pattern character mismatch', path)
    end
    return value
end

function V.check_Schema68(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
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
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
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
    R.check(#value >= 0 and #value <= 40, 'pattern length mismatch', path)
    for index = 1, #value do
        local byte = value:byte(index)
        R.check((byte >= 97 and byte <= 122) or (byte >= 65 and byte <= 90) or (byte >= 48 and byte <= 57) or (byte >= 45 and byte <= 45) or (byte >= 46 and byte <= 46) or (byte >= 95 and byte <= 95) or (byte >= 44 and byte <= 44) or (byte >= 124 and byte <= 124), 'pattern character mismatch', path)
    end
    return value
end

function V.check_Schema74(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema75(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema76(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema77(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema78(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.check(#value >= 0 and #value <= 40, 'pattern length mismatch', path)
    for index = 1, #value do
        local byte = value:byte(index)
        R.check((byte >= 97 and byte <= 122) or (byte >= 65 and byte <= 90) or (byte >= 48 and byte <= 57) or (byte >= 45 and byte <= 45) or (byte >= 46 and byte <= 46) or (byte >= 95 and byte <= 95) or (byte >= 44 and byte <= 44) or (byte >= 124 and byte <= 124), 'pattern character mismatch', path)
    end
    return value
end

function V.check_Schema79(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema80(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema81(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema82(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema83(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema84(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema85(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema86(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema87(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
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
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema91(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema92(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema93(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema94(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema95(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema96(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema97(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema98(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"Encode", "Embed", "External", "Hls", "Drop", }), path)
    V.check_SubtitleDeliveryMethod(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema99(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema100(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema101(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema102(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema103(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema104(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema105(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema106(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema107(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema108(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.check(#value >= 0 and #value <= 40, 'pattern length mismatch', path)
    for index = 1, #value do
        local byte = value:byte(index)
        R.check((byte >= 97 and byte <= 122) or (byte >= 65 and byte <= 90) or (byte >= 48 and byte <= 57) or (byte >= 45 and byte <= 45) or (byte >= 46 and byte <= 46) or (byte >= 95 and byte <= 95) or (byte >= 44 and byte <= 44) or (byte >= 124 and byte <= 124), 'pattern character mismatch', path)
    end
    return value
end

function V.check_Schema109(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.check(#value >= 0 and #value <= 40, 'pattern length mismatch', path)
    for index = 1, #value do
        local byte = value:byte(index)
        R.check((byte >= 97 and byte <= 122) or (byte >= 65 and byte <= 90) or (byte >= 48 and byte <= 57) or (byte >= 45 and byte <= 45) or (byte >= 46 and byte <= 46) or (byte >= 95 and byte <= 95) or (byte >= 44 and byte <= 44) or (byte >= 124 and byte <= 124), 'pattern character mismatch', path)
    end
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
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema112(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema113(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"Streaming", "Static", }), path)
    V.check_EncodingContext(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema114(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema264(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema115(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
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
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema118(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema119(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema120(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema121(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema122(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema123(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema124(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema125(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema126(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema127(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema128(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema129(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema130(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_LocationType(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema131(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_LocationType(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema132(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
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
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema135(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
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

function V.check_Schema138(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema139(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
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
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema142(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema143(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema144(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema145(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema146(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
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
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema149(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema265(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema150(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema151(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema152(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema153(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema154(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_SortOrder(J, child, path .. '/' .. index, state, depth + 1)
    end
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
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ItemFields(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema157(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_BaseItemKind(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema158(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_BaseItemKind(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema159(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ItemFilter(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema160(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema161(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_MediaType(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema162(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ImageType(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema163(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ItemSortBy(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema164(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema165(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema266(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema166(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema267(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema167(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema268(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema168(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema269(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema169(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema170(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema171(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ImageType(J, child, path .. '/' .. index, state, depth + 1)
    end
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

function V.check_Schema173(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema270(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema174(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema271(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema175(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema272(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema176(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema273(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema177(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema274(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema178(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema275(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema179(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema276(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema180(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema277(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema181(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema278(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema182(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema279(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema183(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema280(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema184(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_VideoType(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema185(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema186(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema187(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema188(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema189(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema190(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema191(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema192(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema193(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema194(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema195(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_SeriesStatus(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema196(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema197(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
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
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema281(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema200(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema282(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema201(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema202(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema203(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema204(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
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
    R.enum(J, value, J.array({"Primary", "Art", "Backdrop", "Banner", "Logo", "Thumb", "Disc", "Box", "Screenshot", "Menu", "Chapter", "BoxRear", "Profile", }), path)
    V.check_ImageType(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema207(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema208(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema209(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema210(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema211(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema212(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema213(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
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
    R.enum(J, value, J.array({"Bmp", "Gif", "Jpg", "Png", "Webp", "Svg", }), path)
    V.check_ImageFormat(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema216(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema217(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema218(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
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
    return value
end

function V.check_Schema221(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
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

function V.check_Schema223(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_VirtualFolderInfo(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema224(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_VirtualFolderInfo(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema225(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_VirtualFolderInfo(J, child, path .. '/' .. index, state, depth + 1)
    end
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
    return value
end

function V.check_Schema228(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema229(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema230(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ItemFields(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema231(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
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
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema234(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ImageType(J, child, path .. '/' .. index, state, depth + 1)
    end
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

function V.check_Schema237(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema238(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema239(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema240(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema241(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_CollectionType(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema242(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema243(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    V.check_AuthenticateUserByName(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema244(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    V.check_AuthenticateUserByName(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema245(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    V.check_AuthenticateUserByName(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema246(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema247(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema248(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema249(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema250(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema251(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema252(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
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

function V.check_Schema256(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema257(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema258(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema259(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema260(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
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
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema263(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema264(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema265(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema266(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema267(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema268(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema269(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema270(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema271(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema272(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema273(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema274(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
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
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
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
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema281(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema282(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_BaseItemDtoQueryResult(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["Items"] ~= nil then
        V.check_Schema284(J, value["Items"], R.child_path(path, "Items"), state, depth + 1)
    end
    if value["StartIndex"] ~= nil then
        V.check_Schema285(J, value["StartIndex"], R.child_path(path, "StartIndex"), state, depth + 1)
    end
    if value["TotalRecordCount"] ~= nil then
        V.check_Schema286(J, value["TotalRecordCount"], R.child_path(path, "TotalRecordCount"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "Items" and key ~= "StartIndex" and key ~= "TotalRecordCount" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema284(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_BaseItemDto(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema285(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema286(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_LyricDto(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["Lyrics"] ~= nil then
        V.check_Schema288(J, value["Lyrics"], R.child_path(path, "Lyrics"), state, depth + 1)
    end
    if value["Metadata"] ~= nil then
        V.check_Schema289(J, value["Metadata"], R.child_path(path, "Metadata"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "Lyrics" and key ~= "Metadata" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema288(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_LyricLine(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema289(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    V.check_LyricMetadata(J, value, path, state, depth + 1)
    return value
end

function V.check_ProblemDetails(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["detail"] ~= nil then
        V.check_Schema291(J, value["detail"], R.child_path(path, "detail"), state, depth + 1)
    end
    if value["instance"] ~= nil then
        V.check_Schema292(J, value["instance"], R.child_path(path, "instance"), state, depth + 1)
    end
    if value["status"] ~= nil then
        V.check_Schema293(J, value["status"], R.child_path(path, "status"), state, depth + 1)
    end
    if value["title"] ~= nil then
        V.check_Schema294(J, value["title"], R.child_path(path, "title"), state, depth + 1)
    end
    if value["type"] ~= nil then
        V.check_Schema295(J, value["type"], R.child_path(path, "type"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "detail" and key ~= "instance" and key ~= "status" and key ~= "title" and key ~= "type" then
            V.check_Schema296(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema291(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema292(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema293(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema294(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema295(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema296(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    return value
end

function V.check_BaseItemDto(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["AirDays"] ~= nil then
        V.check_Schema298(J, value["AirDays"], R.child_path(path, "AirDays"), state, depth + 1)
    end
    if value["AirTime"] ~= nil then
        V.check_Schema299(J, value["AirTime"], R.child_path(path, "AirTime"), state, depth + 1)
    end
    if value["AirsAfterSeasonNumber"] ~= nil then
        V.check_Schema300(J, value["AirsAfterSeasonNumber"], R.child_path(path, "AirsAfterSeasonNumber"), state, depth + 1)
    end
    if value["AirsBeforeEpisodeNumber"] ~= nil then
        V.check_Schema301(J, value["AirsBeforeEpisodeNumber"], R.child_path(path, "AirsBeforeEpisodeNumber"), state, depth + 1)
    end
    if value["AirsBeforeSeasonNumber"] ~= nil then
        V.check_Schema302(J, value["AirsBeforeSeasonNumber"], R.child_path(path, "AirsBeforeSeasonNumber"), state, depth + 1)
    end
    if value["Album"] ~= nil then
        V.check_Schema303(J, value["Album"], R.child_path(path, "Album"), state, depth + 1)
    end
    if value["AlbumArtist"] ~= nil then
        V.check_Schema304(J, value["AlbumArtist"], R.child_path(path, "AlbumArtist"), state, depth + 1)
    end
    if value["AlbumArtists"] ~= nil then
        V.check_Schema305(J, value["AlbumArtists"], R.child_path(path, "AlbumArtists"), state, depth + 1)
    end
    if value["AlbumCount"] ~= nil then
        V.check_Schema306(J, value["AlbumCount"], R.child_path(path, "AlbumCount"), state, depth + 1)
    end
    if value["AlbumId"] ~= nil then
        V.check_Schema307(J, value["AlbumId"], R.child_path(path, "AlbumId"), state, depth + 1)
    end
    if value["AlbumPrimaryImageTag"] ~= nil then
        V.check_Schema308(J, value["AlbumPrimaryImageTag"], R.child_path(path, "AlbumPrimaryImageTag"), state, depth + 1)
    end
    if value["Altitude"] ~= nil then
        V.check_Schema309(J, value["Altitude"], R.child_path(path, "Altitude"), state, depth + 1)
    end
    if value["Aperture"] ~= nil then
        V.check_Schema310(J, value["Aperture"], R.child_path(path, "Aperture"), state, depth + 1)
    end
    if value["ArtistCount"] ~= nil then
        V.check_Schema311(J, value["ArtistCount"], R.child_path(path, "ArtistCount"), state, depth + 1)
    end
    if value["ArtistItems"] ~= nil then
        V.check_Schema312(J, value["ArtistItems"], R.child_path(path, "ArtistItems"), state, depth + 1)
    end
    if value["Artists"] ~= nil then
        V.check_Schema313(J, value["Artists"], R.child_path(path, "Artists"), state, depth + 1)
    end
    if value["AspectRatio"] ~= nil then
        V.check_Schema314(J, value["AspectRatio"], R.child_path(path, "AspectRatio"), state, depth + 1)
    end
    if value["Audio"] ~= nil then
        V.check_Schema315(J, value["Audio"], R.child_path(path, "Audio"), state, depth + 1)
    end
    if value["BackdropImageTags"] ~= nil then
        V.check_Schema316(J, value["BackdropImageTags"], R.child_path(path, "BackdropImageTags"), state, depth + 1)
    end
    if value["CameraMake"] ~= nil then
        V.check_Schema317(J, value["CameraMake"], R.child_path(path, "CameraMake"), state, depth + 1)
    end
    if value["CameraModel"] ~= nil then
        V.check_Schema318(J, value["CameraModel"], R.child_path(path, "CameraModel"), state, depth + 1)
    end
    if value["CanDelete"] ~= nil then
        V.check_Schema319(J, value["CanDelete"], R.child_path(path, "CanDelete"), state, depth + 1)
    end
    if value["CanDownload"] ~= nil then
        V.check_Schema320(J, value["CanDownload"], R.child_path(path, "CanDownload"), state, depth + 1)
    end
    if value["ChannelId"] ~= nil then
        V.check_Schema321(J, value["ChannelId"], R.child_path(path, "ChannelId"), state, depth + 1)
    end
    if value["ChannelName"] ~= nil then
        V.check_Schema322(J, value["ChannelName"], R.child_path(path, "ChannelName"), state, depth + 1)
    end
    if value["ChannelNumber"] ~= nil then
        V.check_Schema323(J, value["ChannelNumber"], R.child_path(path, "ChannelNumber"), state, depth + 1)
    end
    if value["ChannelPrimaryImageTag"] ~= nil then
        V.check_Schema324(J, value["ChannelPrimaryImageTag"], R.child_path(path, "ChannelPrimaryImageTag"), state, depth + 1)
    end
    if value["ChannelType"] ~= nil then
        V.check_Schema325(J, value["ChannelType"], R.child_path(path, "ChannelType"), state, depth + 1)
    end
    if value["Chapters"] ~= nil then
        V.check_Schema326(J, value["Chapters"], R.child_path(path, "Chapters"), state, depth + 1)
    end
    if value["ChildCount"] ~= nil then
        V.check_Schema327(J, value["ChildCount"], R.child_path(path, "ChildCount"), state, depth + 1)
    end
    if value["CollectionType"] ~= nil then
        V.check_Schema328(J, value["CollectionType"], R.child_path(path, "CollectionType"), state, depth + 1)
    end
    if value["CommunityRating"] ~= nil then
        V.check_Schema329(J, value["CommunityRating"], R.child_path(path, "CommunityRating"), state, depth + 1)
    end
    if value["CompletionPercentage"] ~= nil then
        V.check_Schema330(J, value["CompletionPercentage"], R.child_path(path, "CompletionPercentage"), state, depth + 1)
    end
    if value["Container"] ~= nil then
        V.check_Schema331(J, value["Container"], R.child_path(path, "Container"), state, depth + 1)
    end
    if value["CriticRating"] ~= nil then
        V.check_Schema332(J, value["CriticRating"], R.child_path(path, "CriticRating"), state, depth + 1)
    end
    if value["CumulativeRunTimeTicks"] ~= nil then
        V.check_Schema333(J, value["CumulativeRunTimeTicks"], R.child_path(path, "CumulativeRunTimeTicks"), state, depth + 1)
    end
    if value["CurrentProgram"] ~= nil then
        V.check_Schema334(J, value["CurrentProgram"], R.child_path(path, "CurrentProgram"), state, depth + 1)
    end
    if value["CustomRating"] ~= nil then
        V.check_Schema335(J, value["CustomRating"], R.child_path(path, "CustomRating"), state, depth + 1)
    end
    if value["DateCreated"] ~= nil then
        V.check_Schema336(J, value["DateCreated"], R.child_path(path, "DateCreated"), state, depth + 1)
    end
    if value["DateLastMediaAdded"] ~= nil then
        V.check_Schema337(J, value["DateLastMediaAdded"], R.child_path(path, "DateLastMediaAdded"), state, depth + 1)
    end
    if value["DisplayOrder"] ~= nil then
        V.check_Schema338(J, value["DisplayOrder"], R.child_path(path, "DisplayOrder"), state, depth + 1)
    end
    if value["DisplayPreferencesId"] ~= nil then
        V.check_Schema339(J, value["DisplayPreferencesId"], R.child_path(path, "DisplayPreferencesId"), state, depth + 1)
    end
    if value["EnableMediaSourceDisplay"] ~= nil then
        V.check_Schema340(J, value["EnableMediaSourceDisplay"], R.child_path(path, "EnableMediaSourceDisplay"), state, depth + 1)
    end
    if value["EndDate"] ~= nil then
        V.check_Schema341(J, value["EndDate"], R.child_path(path, "EndDate"), state, depth + 1)
    end
    if value["EpisodeCount"] ~= nil then
        V.check_Schema342(J, value["EpisodeCount"], R.child_path(path, "EpisodeCount"), state, depth + 1)
    end
    if value["EpisodeTitle"] ~= nil then
        V.check_Schema343(J, value["EpisodeTitle"], R.child_path(path, "EpisodeTitle"), state, depth + 1)
    end
    if value["Etag"] ~= nil then
        V.check_Schema344(J, value["Etag"], R.child_path(path, "Etag"), state, depth + 1)
    end
    if value["ExposureTime"] ~= nil then
        V.check_Schema345(J, value["ExposureTime"], R.child_path(path, "ExposureTime"), state, depth + 1)
    end
    if value["ExternalUrls"] ~= nil then
        V.check_Schema346(J, value["ExternalUrls"], R.child_path(path, "ExternalUrls"), state, depth + 1)
    end
    if value["ExtraType"] ~= nil then
        V.check_Schema347(J, value["ExtraType"], R.child_path(path, "ExtraType"), state, depth + 1)
    end
    if value["FocalLength"] ~= nil then
        V.check_Schema348(J, value["FocalLength"], R.child_path(path, "FocalLength"), state, depth + 1)
    end
    if value["ForcedSortName"] ~= nil then
        V.check_Schema349(J, value["ForcedSortName"], R.child_path(path, "ForcedSortName"), state, depth + 1)
    end
    if value["GenreItems"] ~= nil then
        V.check_Schema350(J, value["GenreItems"], R.child_path(path, "GenreItems"), state, depth + 1)
    end
    if value["Genres"] ~= nil then
        V.check_Schema351(J, value["Genres"], R.child_path(path, "Genres"), state, depth + 1)
    end
    if value["HasLyrics"] ~= nil then
        V.check_Schema352(J, value["HasLyrics"], R.child_path(path, "HasLyrics"), state, depth + 1)
    end
    if value["HasSubtitles"] ~= nil then
        V.check_Schema353(J, value["HasSubtitles"], R.child_path(path, "HasSubtitles"), state, depth + 1)
    end
    if value["Height"] ~= nil then
        V.check_Schema354(J, value["Height"], R.child_path(path, "Height"), state, depth + 1)
    end
    if value["Id"] ~= nil then
        V.check_Schema355(J, value["Id"], R.child_path(path, "Id"), state, depth + 1)
    end
    if value["ImageBlurHashes"] ~= nil then
        V.check_Schema356(J, value["ImageBlurHashes"], R.child_path(path, "ImageBlurHashes"), state, depth + 1)
    end
    if value["ImageOrientation"] ~= nil then
        V.check_Schema357(J, value["ImageOrientation"], R.child_path(path, "ImageOrientation"), state, depth + 1)
    end
    if value["ImageTags"] ~= nil then
        V.check_Schema358(J, value["ImageTags"], R.child_path(path, "ImageTags"), state, depth + 1)
    end
    if value["IndexNumber"] ~= nil then
        V.check_Schema359(J, value["IndexNumber"], R.child_path(path, "IndexNumber"), state, depth + 1)
    end
    if value["IndexNumberEnd"] ~= nil then
        V.check_Schema360(J, value["IndexNumberEnd"], R.child_path(path, "IndexNumberEnd"), state, depth + 1)
    end
    if value["IsFolder"] ~= nil then
        V.check_Schema361(J, value["IsFolder"], R.child_path(path, "IsFolder"), state, depth + 1)
    end
    if value["IsHD"] ~= nil then
        V.check_Schema362(J, value["IsHD"], R.child_path(path, "IsHD"), state, depth + 1)
    end
    if value["IsKids"] ~= nil then
        V.check_Schema363(J, value["IsKids"], R.child_path(path, "IsKids"), state, depth + 1)
    end
    if value["IsLive"] ~= nil then
        V.check_Schema364(J, value["IsLive"], R.child_path(path, "IsLive"), state, depth + 1)
    end
    if value["IsMovie"] ~= nil then
        V.check_Schema365(J, value["IsMovie"], R.child_path(path, "IsMovie"), state, depth + 1)
    end
    if value["IsNews"] ~= nil then
        V.check_Schema366(J, value["IsNews"], R.child_path(path, "IsNews"), state, depth + 1)
    end
    if value["IsPlaceHolder"] ~= nil then
        V.check_Schema367(J, value["IsPlaceHolder"], R.child_path(path, "IsPlaceHolder"), state, depth + 1)
    end
    if value["IsPremiere"] ~= nil then
        V.check_Schema368(J, value["IsPremiere"], R.child_path(path, "IsPremiere"), state, depth + 1)
    end
    if value["IsRepeat"] ~= nil then
        V.check_Schema369(J, value["IsRepeat"], R.child_path(path, "IsRepeat"), state, depth + 1)
    end
    if value["IsSeries"] ~= nil then
        V.check_Schema370(J, value["IsSeries"], R.child_path(path, "IsSeries"), state, depth + 1)
    end
    if value["IsSports"] ~= nil then
        V.check_Schema371(J, value["IsSports"], R.child_path(path, "IsSports"), state, depth + 1)
    end
    if value["IsoSpeedRating"] ~= nil then
        V.check_Schema372(J, value["IsoSpeedRating"], R.child_path(path, "IsoSpeedRating"), state, depth + 1)
    end
    if value["IsoType"] ~= nil then
        V.check_Schema373(J, value["IsoType"], R.child_path(path, "IsoType"), state, depth + 1)
    end
    if value["Latitude"] ~= nil then
        V.check_Schema374(J, value["Latitude"], R.child_path(path, "Latitude"), state, depth + 1)
    end
    if value["LocalTrailerCount"] ~= nil then
        V.check_Schema375(J, value["LocalTrailerCount"], R.child_path(path, "LocalTrailerCount"), state, depth + 1)
    end
    if value["LocationType"] ~= nil then
        V.check_Schema376(J, value["LocationType"], R.child_path(path, "LocationType"), state, depth + 1)
    end
    if value["LockData"] ~= nil then
        V.check_Schema377(J, value["LockData"], R.child_path(path, "LockData"), state, depth + 1)
    end
    if value["LockedFields"] ~= nil then
        V.check_Schema378(J, value["LockedFields"], R.child_path(path, "LockedFields"), state, depth + 1)
    end
    if value["Longitude"] ~= nil then
        V.check_Schema379(J, value["Longitude"], R.child_path(path, "Longitude"), state, depth + 1)
    end
    if value["MediaSourceCount"] ~= nil then
        V.check_Schema380(J, value["MediaSourceCount"], R.child_path(path, "MediaSourceCount"), state, depth + 1)
    end
    if value["MediaSources"] ~= nil then
        V.check_Schema381(J, value["MediaSources"], R.child_path(path, "MediaSources"), state, depth + 1)
    end
    if value["MediaStreams"] ~= nil then
        V.check_Schema382(J, value["MediaStreams"], R.child_path(path, "MediaStreams"), state, depth + 1)
    end
    if value["MediaType"] ~= nil then
        V.check_Schema383(J, value["MediaType"], R.child_path(path, "MediaType"), state, depth + 1)
    end
    if value["MovieCount"] ~= nil then
        V.check_Schema384(J, value["MovieCount"], R.child_path(path, "MovieCount"), state, depth + 1)
    end
    if value["MusicVideoCount"] ~= nil then
        V.check_Schema385(J, value["MusicVideoCount"], R.child_path(path, "MusicVideoCount"), state, depth + 1)
    end
    if value["Name"] ~= nil then
        V.check_Schema386(J, value["Name"], R.child_path(path, "Name"), state, depth + 1)
    end
    if value["NormalizationGain"] ~= nil then
        V.check_Schema387(J, value["NormalizationGain"], R.child_path(path, "NormalizationGain"), state, depth + 1)
    end
    if value["Number"] ~= nil then
        V.check_Schema388(J, value["Number"], R.child_path(path, "Number"), state, depth + 1)
    end
    if value["OfficialRating"] ~= nil then
        V.check_Schema389(J, value["OfficialRating"], R.child_path(path, "OfficialRating"), state, depth + 1)
    end
    if value["OriginalTitle"] ~= nil then
        V.check_Schema390(J, value["OriginalTitle"], R.child_path(path, "OriginalTitle"), state, depth + 1)
    end
    if value["Overview"] ~= nil then
        V.check_Schema391(J, value["Overview"], R.child_path(path, "Overview"), state, depth + 1)
    end
    if value["ParentArtImageTag"] ~= nil then
        V.check_Schema392(J, value["ParentArtImageTag"], R.child_path(path, "ParentArtImageTag"), state, depth + 1)
    end
    if value["ParentArtItemId"] ~= nil then
        V.check_Schema393(J, value["ParentArtItemId"], R.child_path(path, "ParentArtItemId"), state, depth + 1)
    end
    if value["ParentBackdropImageTags"] ~= nil then
        V.check_Schema394(J, value["ParentBackdropImageTags"], R.child_path(path, "ParentBackdropImageTags"), state, depth + 1)
    end
    if value["ParentBackdropItemId"] ~= nil then
        V.check_Schema395(J, value["ParentBackdropItemId"], R.child_path(path, "ParentBackdropItemId"), state, depth + 1)
    end
    if value["ParentId"] ~= nil then
        V.check_Schema396(J, value["ParentId"], R.child_path(path, "ParentId"), state, depth + 1)
    end
    if value["ParentIndexNumber"] ~= nil then
        V.check_Schema397(J, value["ParentIndexNumber"], R.child_path(path, "ParentIndexNumber"), state, depth + 1)
    end
    if value["ParentLogoImageTag"] ~= nil then
        V.check_Schema398(J, value["ParentLogoImageTag"], R.child_path(path, "ParentLogoImageTag"), state, depth + 1)
    end
    if value["ParentLogoItemId"] ~= nil then
        V.check_Schema399(J, value["ParentLogoItemId"], R.child_path(path, "ParentLogoItemId"), state, depth + 1)
    end
    if value["ParentPrimaryImageItemId"] ~= nil then
        V.check_Schema400(J, value["ParentPrimaryImageItemId"], R.child_path(path, "ParentPrimaryImageItemId"), state, depth + 1)
    end
    if value["ParentPrimaryImageTag"] ~= nil then
        V.check_Schema401(J, value["ParentPrimaryImageTag"], R.child_path(path, "ParentPrimaryImageTag"), state, depth + 1)
    end
    if value["ParentThumbImageTag"] ~= nil then
        V.check_Schema402(J, value["ParentThumbImageTag"], R.child_path(path, "ParentThumbImageTag"), state, depth + 1)
    end
    if value["ParentThumbItemId"] ~= nil then
        V.check_Schema403(J, value["ParentThumbItemId"], R.child_path(path, "ParentThumbItemId"), state, depth + 1)
    end
    if value["PartCount"] ~= nil then
        V.check_Schema404(J, value["PartCount"], R.child_path(path, "PartCount"), state, depth + 1)
    end
    if value["Path"] ~= nil then
        V.check_Schema405(J, value["Path"], R.child_path(path, "Path"), state, depth + 1)
    end
    if value["People"] ~= nil then
        V.check_Schema406(J, value["People"], R.child_path(path, "People"), state, depth + 1)
    end
    if value["PlayAccess"] ~= nil then
        V.check_Schema407(J, value["PlayAccess"], R.child_path(path, "PlayAccess"), state, depth + 1)
    end
    if value["PlaylistItemId"] ~= nil then
        V.check_Schema408(J, value["PlaylistItemId"], R.child_path(path, "PlaylistItemId"), state, depth + 1)
    end
    if value["PreferredMetadataCountryCode"] ~= nil then
        V.check_Schema409(J, value["PreferredMetadataCountryCode"], R.child_path(path, "PreferredMetadataCountryCode"), state, depth + 1)
    end
    if value["PreferredMetadataLanguage"] ~= nil then
        V.check_Schema410(J, value["PreferredMetadataLanguage"], R.child_path(path, "PreferredMetadataLanguage"), state, depth + 1)
    end
    if value["PremiereDate"] ~= nil then
        V.check_Schema411(J, value["PremiereDate"], R.child_path(path, "PremiereDate"), state, depth + 1)
    end
    if value["PrimaryImageAspectRatio"] ~= nil then
        V.check_Schema412(J, value["PrimaryImageAspectRatio"], R.child_path(path, "PrimaryImageAspectRatio"), state, depth + 1)
    end
    if value["ProductionLocations"] ~= nil then
        V.check_Schema413(J, value["ProductionLocations"], R.child_path(path, "ProductionLocations"), state, depth + 1)
    end
    if value["ProductionYear"] ~= nil then
        V.check_Schema414(J, value["ProductionYear"], R.child_path(path, "ProductionYear"), state, depth + 1)
    end
    if value["ProgramCount"] ~= nil then
        V.check_Schema415(J, value["ProgramCount"], R.child_path(path, "ProgramCount"), state, depth + 1)
    end
    if value["ProgramId"] ~= nil then
        V.check_Schema416(J, value["ProgramId"], R.child_path(path, "ProgramId"), state, depth + 1)
    end
    if value["ProviderIds"] ~= nil then
        V.check_Schema417(J, value["ProviderIds"], R.child_path(path, "ProviderIds"), state, depth + 1)
    end
    if value["RecursiveItemCount"] ~= nil then
        V.check_Schema418(J, value["RecursiveItemCount"], R.child_path(path, "RecursiveItemCount"), state, depth + 1)
    end
    if value["RemoteTrailers"] ~= nil then
        V.check_Schema419(J, value["RemoteTrailers"], R.child_path(path, "RemoteTrailers"), state, depth + 1)
    end
    if value["RunTimeTicks"] ~= nil then
        V.check_Schema420(J, value["RunTimeTicks"], R.child_path(path, "RunTimeTicks"), state, depth + 1)
    end
    if value["ScreenshotImageTags"] ~= nil then
        V.check_Schema421(J, value["ScreenshotImageTags"], R.child_path(path, "ScreenshotImageTags"), state, depth + 1)
    end
    if value["SeasonId"] ~= nil then
        V.check_Schema422(J, value["SeasonId"], R.child_path(path, "SeasonId"), state, depth + 1)
    end
    if value["SeasonName"] ~= nil then
        V.check_Schema423(J, value["SeasonName"], R.child_path(path, "SeasonName"), state, depth + 1)
    end
    if value["SeriesCount"] ~= nil then
        V.check_Schema424(J, value["SeriesCount"], R.child_path(path, "SeriesCount"), state, depth + 1)
    end
    if value["SeriesId"] ~= nil then
        V.check_Schema425(J, value["SeriesId"], R.child_path(path, "SeriesId"), state, depth + 1)
    end
    if value["SeriesName"] ~= nil then
        V.check_Schema426(J, value["SeriesName"], R.child_path(path, "SeriesName"), state, depth + 1)
    end
    if value["SeriesPrimaryImageTag"] ~= nil then
        V.check_Schema427(J, value["SeriesPrimaryImageTag"], R.child_path(path, "SeriesPrimaryImageTag"), state, depth + 1)
    end
    if value["SeriesStudio"] ~= nil then
        V.check_Schema428(J, value["SeriesStudio"], R.child_path(path, "SeriesStudio"), state, depth + 1)
    end
    if value["SeriesThumbImageTag"] ~= nil then
        V.check_Schema429(J, value["SeriesThumbImageTag"], R.child_path(path, "SeriesThumbImageTag"), state, depth + 1)
    end
    if value["SeriesTimerId"] ~= nil then
        V.check_Schema430(J, value["SeriesTimerId"], R.child_path(path, "SeriesTimerId"), state, depth + 1)
    end
    if value["ServerId"] ~= nil then
        V.check_Schema431(J, value["ServerId"], R.child_path(path, "ServerId"), state, depth + 1)
    end
    if value["ShutterSpeed"] ~= nil then
        V.check_Schema432(J, value["ShutterSpeed"], R.child_path(path, "ShutterSpeed"), state, depth + 1)
    end
    if value["Software"] ~= nil then
        V.check_Schema433(J, value["Software"], R.child_path(path, "Software"), state, depth + 1)
    end
    if value["SongCount"] ~= nil then
        V.check_Schema434(J, value["SongCount"], R.child_path(path, "SongCount"), state, depth + 1)
    end
    if value["SortName"] ~= nil then
        V.check_Schema435(J, value["SortName"], R.child_path(path, "SortName"), state, depth + 1)
    end
    if value["SourceType"] ~= nil then
        V.check_Schema436(J, value["SourceType"], R.child_path(path, "SourceType"), state, depth + 1)
    end
    if value["SpecialFeatureCount"] ~= nil then
        V.check_Schema437(J, value["SpecialFeatureCount"], R.child_path(path, "SpecialFeatureCount"), state, depth + 1)
    end
    if value["StartDate"] ~= nil then
        V.check_Schema438(J, value["StartDate"], R.child_path(path, "StartDate"), state, depth + 1)
    end
    if value["Status"] ~= nil then
        V.check_Schema439(J, value["Status"], R.child_path(path, "Status"), state, depth + 1)
    end
    if value["Studios"] ~= nil then
        V.check_Schema440(J, value["Studios"], R.child_path(path, "Studios"), state, depth + 1)
    end
    if value["Taglines"] ~= nil then
        V.check_Schema441(J, value["Taglines"], R.child_path(path, "Taglines"), state, depth + 1)
    end
    if value["Tags"] ~= nil then
        V.check_Schema442(J, value["Tags"], R.child_path(path, "Tags"), state, depth + 1)
    end
    if value["TimerId"] ~= nil then
        V.check_Schema443(J, value["TimerId"], R.child_path(path, "TimerId"), state, depth + 1)
    end
    if value["TrailerCount"] ~= nil then
        V.check_Schema444(J, value["TrailerCount"], R.child_path(path, "TrailerCount"), state, depth + 1)
    end
    if value["Trickplay"] ~= nil then
        V.check_Schema445(J, value["Trickplay"], R.child_path(path, "Trickplay"), state, depth + 1)
    end
    if value["Type"] ~= nil then
        V.check_Schema446(J, value["Type"], R.child_path(path, "Type"), state, depth + 1)
    end
    if value["UserData"] ~= nil then
        V.check_Schema447(J, value["UserData"], R.child_path(path, "UserData"), state, depth + 1)
    end
    if value["Video3DFormat"] ~= nil then
        V.check_Schema448(J, value["Video3DFormat"], R.child_path(path, "Video3DFormat"), state, depth + 1)
    end
    if value["VideoType"] ~= nil then
        V.check_Schema449(J, value["VideoType"], R.child_path(path, "VideoType"), state, depth + 1)
    end
    if value["Width"] ~= nil then
        V.check_Schema450(J, value["Width"], R.child_path(path, "Width"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "AirDays" and key ~= "AirTime" and key ~= "AirsAfterSeasonNumber" and key ~= "AirsBeforeEpisodeNumber" and key ~= "AirsBeforeSeasonNumber" and key ~= "Album" and key ~= "AlbumArtist" and key ~= "AlbumArtists" and key ~= "AlbumCount" and key ~= "AlbumId" and key ~= "AlbumPrimaryImageTag" and key ~= "Altitude" and key ~= "Aperture" and key ~= "ArtistCount" and key ~= "ArtistItems" and key ~= "Artists" and key ~= "AspectRatio" and key ~= "Audio" and key ~= "BackdropImageTags" and key ~= "CameraMake" and key ~= "CameraModel" and key ~= "CanDelete" and key ~= "CanDownload" and key ~= "ChannelId" and key ~= "ChannelName" and key ~= "ChannelNumber" and key ~= "ChannelPrimaryImageTag" and key ~= "ChannelType" and key ~= "Chapters" and key ~= "ChildCount" and key ~= "CollectionType" and key ~= "CommunityRating" and key ~= "CompletionPercentage" and key ~= "Container" and key ~= "CriticRating" and key ~= "CumulativeRunTimeTicks" and key ~= "CurrentProgram" and key ~= "CustomRating" and key ~= "DateCreated" and key ~= "DateLastMediaAdded" and key ~= "DisplayOrder" and key ~= "DisplayPreferencesId" and key ~= "EnableMediaSourceDisplay" and key ~= "EndDate" and key ~= "EpisodeCount" and key ~= "EpisodeTitle" and key ~= "Etag" and key ~= "ExposureTime" and key ~= "ExternalUrls" and key ~= "ExtraType" and key ~= "FocalLength" and key ~= "ForcedSortName" and key ~= "GenreItems" and key ~= "Genres" and key ~= "HasLyrics" and key ~= "HasSubtitles" and key ~= "Height" and key ~= "Id" and key ~= "ImageBlurHashes" and key ~= "ImageOrientation" and key ~= "ImageTags" and key ~= "IndexNumber" and key ~= "IndexNumberEnd" and key ~= "IsFolder" and key ~= "IsHD" and key ~= "IsKids" and key ~= "IsLive" and key ~= "IsMovie" and key ~= "IsNews" and key ~= "IsPlaceHolder" and key ~= "IsPremiere" and key ~= "IsRepeat" and key ~= "IsSeries" and key ~= "IsSports" and key ~= "IsoSpeedRating" and key ~= "IsoType" and key ~= "Latitude" and key ~= "LocalTrailerCount" and key ~= "LocationType" and key ~= "LockData" and key ~= "LockedFields" and key ~= "Longitude" and key ~= "MediaSourceCount" and key ~= "MediaSources" and key ~= "MediaStreams" and key ~= "MediaType" and key ~= "MovieCount" and key ~= "MusicVideoCount" and key ~= "Name" and key ~= "NormalizationGain" and key ~= "Number" and key ~= "OfficialRating" and key ~= "OriginalTitle" and key ~= "Overview" and key ~= "ParentArtImageTag" and key ~= "ParentArtItemId" and key ~= "ParentBackdropImageTags" and key ~= "ParentBackdropItemId" and key ~= "ParentId" and key ~= "ParentIndexNumber" and key ~= "ParentLogoImageTag" and key ~= "ParentLogoItemId" and key ~= "ParentPrimaryImageItemId" and key ~= "ParentPrimaryImageTag" and key ~= "ParentThumbImageTag" and key ~= "ParentThumbItemId" and key ~= "PartCount" and key ~= "Path" and key ~= "People" and key ~= "PlayAccess" and key ~= "PlaylistItemId" and key ~= "PreferredMetadataCountryCode" and key ~= "PreferredMetadataLanguage" and key ~= "PremiereDate" and key ~= "PrimaryImageAspectRatio" and key ~= "ProductionLocations" and key ~= "ProductionYear" and key ~= "ProgramCount" and key ~= "ProgramId" and key ~= "ProviderIds" and key ~= "RecursiveItemCount" and key ~= "RemoteTrailers" and key ~= "RunTimeTicks" and key ~= "ScreenshotImageTags" and key ~= "SeasonId" and key ~= "SeasonName" and key ~= "SeriesCount" and key ~= "SeriesId" and key ~= "SeriesName" and key ~= "SeriesPrimaryImageTag" and key ~= "SeriesStudio" and key ~= "SeriesThumbImageTag" and key ~= "SeriesTimerId" and key ~= "ServerId" and key ~= "ShutterSpeed" and key ~= "Software" and key ~= "SongCount" and key ~= "SortName" and key ~= "SourceType" and key ~= "SpecialFeatureCount" and key ~= "StartDate" and key ~= "Status" and key ~= "Studios" and key ~= "Taglines" and key ~= "Tags" and key ~= "TimerId" and key ~= "TrailerCount" and key ~= "Trickplay" and key ~= "Type" and key ~= "UserData" and key ~= "Video3DFormat" and key ~= "VideoType" and key ~= "Width" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema298(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_DayOfWeek(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema299(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema300(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema301(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema302(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema303(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema304(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema305(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_NameGuidPair(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema306(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema307(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema308(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema309(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema310(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema311(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema312(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_NameGuidPair(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema313(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema451(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema314(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema315(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.enum(J, value, J.array({"Mono", "Stereo", "Dolby", "DolbyDigital", "Thx", "Atmos", }), path)
    V.check_ProgramAudio(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema316(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema452(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema317(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema318(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema319(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema320(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema321(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema322(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema323(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema324(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema325(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.enum(J, value, J.array({"TV", "Radio", }), path)
    V.check_ChannelType(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema326(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ChapterInfo(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema327(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema328(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.enum(J, value, J.array({"unknown", "movies", "tvshows", "music", "musicvideos", "trailers", "homevideos", "boxsets", "books", "photos", "livetv", "playlists", "folders", }), path)
    V.check_CollectionType(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema329(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema330(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema331(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema332(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema333(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema334(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    V.check_BaseItemDto(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema335(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema336(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema337(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema338(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema339(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema340(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema341(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema342(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema343(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema344(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema345(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema346(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ExternalUrl(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema347(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.enum(J, value, J.array({"Unknown", "Clip", "Trailer", "BehindTheScenes", "DeletedScene", "Interview", "Scene", "Sample", "ThemeSong", "ThemeVideo", "Featurette", "Short", }), path)
    V.check_ExtraType(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema348(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema349(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema350(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_NameGuidPair(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema351(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema453(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema352(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema353(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema354(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
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
    if value == J.null then return value end
    local length = R.object(J, value, path)
    if value["Art"] ~= nil then
        V.check_Schema454(J, value["Art"], R.child_path(path, "Art"), state, depth + 1)
    end
    if value["Backdrop"] ~= nil then
        V.check_Schema455(J, value["Backdrop"], R.child_path(path, "Backdrop"), state, depth + 1)
    end
    if value["Banner"] ~= nil then
        V.check_Schema456(J, value["Banner"], R.child_path(path, "Banner"), state, depth + 1)
    end
    if value["Box"] ~= nil then
        V.check_Schema457(J, value["Box"], R.child_path(path, "Box"), state, depth + 1)
    end
    if value["BoxRear"] ~= nil then
        V.check_Schema458(J, value["BoxRear"], R.child_path(path, "BoxRear"), state, depth + 1)
    end
    if value["Chapter"] ~= nil then
        V.check_Schema459(J, value["Chapter"], R.child_path(path, "Chapter"), state, depth + 1)
    end
    if value["Disc"] ~= nil then
        V.check_Schema460(J, value["Disc"], R.child_path(path, "Disc"), state, depth + 1)
    end
    if value["Logo"] ~= nil then
        V.check_Schema461(J, value["Logo"], R.child_path(path, "Logo"), state, depth + 1)
    end
    if value["Menu"] ~= nil then
        V.check_Schema462(J, value["Menu"], R.child_path(path, "Menu"), state, depth + 1)
    end
    if value["Primary"] ~= nil then
        V.check_Schema463(J, value["Primary"], R.child_path(path, "Primary"), state, depth + 1)
    end
    if value["Profile"] ~= nil then
        V.check_Schema464(J, value["Profile"], R.child_path(path, "Profile"), state, depth + 1)
    end
    if value["Screenshot"] ~= nil then
        V.check_Schema465(J, value["Screenshot"], R.child_path(path, "Screenshot"), state, depth + 1)
    end
    if value["Thumb"] ~= nil then
        V.check_Schema466(J, value["Thumb"], R.child_path(path, "Thumb"), state, depth + 1)
    end
    return value
end

function V.check_Schema357(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.enum(J, value, J.array({"TopLeft", "TopRight", "BottomRight", "BottomLeft", "LeftTop", "RightTop", "RightBottom", "LeftBottom", }), path)
    V.check_ImageOrientation(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema358(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema467(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema359(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema360(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema361(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema362(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema363(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema364(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema365(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema366(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema367(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema368(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema369(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema370(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema371(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema372(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema373(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.enum(J, value, J.array({"Dvd", "BluRay", }), path)
    V.check_IsoType(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema374(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema375(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema376(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.enum(J, value, J.array({"FileSystem", "Remote", "Virtual", "Offline", }), path)
    V.check_LocationType(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema377(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema378(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_MetadataField(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema379(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema380(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema381(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_MediaSourceInfo(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema382(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_MediaStream(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema383(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"Unknown", "Video", "Audio", "Photo", "Book", }), path)
    V.check_MediaType(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema384(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema385(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema386(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema387(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema388(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema389(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema390(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema391(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema392(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema393(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema394(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema468(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema395(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema396(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema397(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema398(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema399(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema400(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema401(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema402(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema403(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema404(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema405(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema406(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_BaseItemPerson(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema407(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.enum(J, value, J.array({"Full", "None", }), path)
    V.check_PlayAccess(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema408(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema409(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema410(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema411(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema412(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema413(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema469(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema414(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema415(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema416(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema417(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema470(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema418(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema419(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_MediaUrl(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema420(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema421(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema471(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema422(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema423(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema424(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema425(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema426(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema427(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema428(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema429(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema430(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema431(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema432(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema433(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema434(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema435(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema436(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema437(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema438(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema439(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema440(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_NameGuidPair(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema441(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema472(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema442(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema473(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema443(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema444(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema445(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema474(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema446(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"AggregateFolder", "Audio", "AudioBook", "BasePluginFolder", "Book", "BoxSet", "Channel", "ChannelFolderItem", "CollectionFolder", "Episode", "Folder", "Genre", "ManualPlaylistsFolder", "Movie", "LiveTvChannel", "LiveTvProgram", "MusicAlbum", "MusicArtist", "MusicGenre", "MusicVideo", "Person", "Photo", "PhotoAlbum", "Playlist", "PlaylistsFolder", "Program", "Recording", "Season", "Series", "Studio", "Trailer", "TvChannel", "TvProgram", "UserRootFolder", "UserView", "Video", "Year", }), path)
    V.check_BaseItemKind(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema447(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    V.check_UserItemDataDto(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema448(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.enum(J, value, J.array({"HalfSideBySide", "FullSideBySide", "FullTopAndBottom", "HalfTopAndBottom", "MVC", }), path)
    V.check_Video3DFormat(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema449(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.enum(J, value, J.array({"VideoFile", "Iso", "Dvd", "BluRay", }), path)
    V.check_VideoType(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema450(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema451(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema452(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
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

function V.check_Schema454(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema475(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema455(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema476(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema456(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema477(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema457(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema478(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema458(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema479(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema459(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema480(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema460(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema481(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema461(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema482(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema462(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema483(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema463(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema484(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema464(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema485(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema465(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema486(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema466(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema487(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema467(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema468(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema469(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema470(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema471(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema472(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema473(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema474(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_TrickplayInfo(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema475(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema476(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema477(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema478(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema479(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema480(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema481(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema482(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema483(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema484(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema485(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema486(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema487(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_EndPointInfo(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["IsInNetwork"] ~= nil then
        V.check_Schema489(J, value["IsInNetwork"], R.child_path(path, "IsInNetwork"), state, depth + 1)
    end
    if value["IsLocal"] ~= nil then
        V.check_Schema490(J, value["IsLocal"], R.child_path(path, "IsLocal"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "IsInNetwork" and key ~= "IsLocal" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema489(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema490(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_UserItemDataDto(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["IsFavorite"] ~= nil then
        V.check_Schema492(J, value["IsFavorite"], R.child_path(path, "IsFavorite"), state, depth + 1)
    end
    if value["ItemId"] ~= nil then
        V.check_Schema493(J, value["ItemId"], R.child_path(path, "ItemId"), state, depth + 1)
    end
    if value["Key"] ~= nil then
        V.check_Schema494(J, value["Key"], R.child_path(path, "Key"), state, depth + 1)
    end
    if value["LastPlayedDate"] ~= nil then
        V.check_Schema495(J, value["LastPlayedDate"], R.child_path(path, "LastPlayedDate"), state, depth + 1)
    end
    if value["Likes"] ~= nil then
        V.check_Schema496(J, value["Likes"], R.child_path(path, "Likes"), state, depth + 1)
    end
    if value["PlayCount"] ~= nil then
        V.check_Schema497(J, value["PlayCount"], R.child_path(path, "PlayCount"), state, depth + 1)
    end
    if value["PlaybackPositionTicks"] ~= nil then
        V.check_Schema498(J, value["PlaybackPositionTicks"], R.child_path(path, "PlaybackPositionTicks"), state, depth + 1)
    end
    if value["Played"] ~= nil then
        V.check_Schema499(J, value["Played"], R.child_path(path, "Played"), state, depth + 1)
    end
    if value["PlayedPercentage"] ~= nil then
        V.check_Schema500(J, value["PlayedPercentage"], R.child_path(path, "PlayedPercentage"), state, depth + 1)
    end
    if value["Rating"] ~= nil then
        V.check_Schema501(J, value["Rating"], R.child_path(path, "Rating"), state, depth + 1)
    end
    if value["UnplayedItemCount"] ~= nil then
        V.check_Schema502(J, value["UnplayedItemCount"], R.child_path(path, "UnplayedItemCount"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "IsFavorite" and key ~= "ItemId" and key ~= "Key" and key ~= "LastPlayedDate" and key ~= "Likes" and key ~= "PlayCount" and key ~= "PlaybackPositionTicks" and key ~= "Played" and key ~= "PlayedPercentage" and key ~= "Rating" and key ~= "UnplayedItemCount" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema492(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema493(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema494(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema495(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema496(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema497(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema498(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema499(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema500(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema501(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema502(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_AuthenticationResult(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["AccessToken"] ~= nil then
        V.check_Schema504(J, value["AccessToken"], R.child_path(path, "AccessToken"), state, depth + 1)
    end
    if value["ServerId"] ~= nil then
        V.check_Schema505(J, value["ServerId"], R.child_path(path, "ServerId"), state, depth + 1)
    end
    if value["SessionInfo"] ~= nil then
        V.check_Schema506(J, value["SessionInfo"], R.child_path(path, "SessionInfo"), state, depth + 1)
    end
    if value["User"] ~= nil then
        V.check_Schema507(J, value["User"], R.child_path(path, "User"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "AccessToken" and key ~= "ServerId" and key ~= "SessionInfo" and key ~= "User" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema504(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema505(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema506(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    V.check_SessionInfoDto(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema507(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    V.check_UserDto(J, value, path, state, depth + 1)
    return value
end

function V.check_ItemFields(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"AirTime", "CanDelete", "CanDownload", "ChannelInfo", "Chapters", "Trickplay", "ChildCount", "CumulativeRunTimeTicks", "CustomRating", "DateCreated", "DateLastMediaAdded", "DisplayPreferencesId", "Etag", "ExternalUrls", "Genres", "HomePageUrl", "ItemCounts", "MediaSourceCount", "MediaSources", "OriginalTitle", "Overview", "ParentId", "Path", "People", "PlayAccess", "ProductionLocations", "ProviderIds", "PrimaryImageAspectRatio", "RecursiveItemCount", "Settings", "ScreenshotImageTags", "SeriesPrimaryImage", "SeriesStudio", "SortName", "SpecialEpisodeNumbers", "Studios", "Taglines", "Tags", "RemoteTrailers", "MediaStreams", "SeasonUserData", "ServiceName", "ThemeSongIds", "ThemeVideoIds", "ExternalEtag", "PresentationUniqueKey", "InheritedParentalRatingValue", "ExternalSeriesId", "SeriesPresentationUniqueKey", "DateLastRefreshed", "DateLastSaved", "RefreshState", "ChannelImage", "EnableMediaSourceDisplay", "Width", "Height", "ExtraIds", "LocalTrailerCount", "IsHD", "SpecialFeatureCount", }), path)
    return value
end

function V.check_BaseItemKind(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"AggregateFolder", "Audio", "AudioBook", "BasePluginFolder", "Book", "BoxSet", "Channel", "ChannelFolderItem", "CollectionFolder", "Episode", "Folder", "Genre", "ManualPlaylistsFolder", "Movie", "LiveTvChannel", "LiveTvProgram", "MusicAlbum", "MusicArtist", "MusicGenre", "MusicVideo", "Person", "Photo", "PhotoAlbum", "Playlist", "PlaylistsFolder", "Program", "Recording", "Season", "Series", "Studio", "Trailer", "TvChannel", "TvProgram", "UserRootFolder", "UserView", "Video", "Year", }), path)
    return value
end

function V.check_ItemFilter(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"IsFolder", "IsNotFolder", "IsUnplayed", "IsPlayed", "IsFavorite", "IsResumable", "Likes", "Dislikes", "IsFavoriteOrLikes", }), path)
    return value
end

function V.check_MediaType(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Unknown", "Video", "Audio", "Photo", "Book", }), path)
    return value
end

function V.check_ImageType(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Primary", "Art", "Backdrop", "Banner", "Logo", "Thumb", "Disc", "Box", "Screenshot", "Menu", "Chapter", "BoxRear", "Profile", }), path)
    return value
end

function V.check_ItemSortBy(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Default", "AiredEpisodeOrder", "Album", "AlbumArtist", "Artist", "DateCreated", "OfficialRating", "DatePlayed", "PremiereDate", "StartDate", "SortName", "Name", "Random", "Runtime", "CommunityRating", "ProductionYear", "PlayCount", "CriticRating", "IsFolder", "IsUnplayed", "IsPlayed", "SeriesSortName", "VideoBitRate", "AirTime", "Studio", "IsFavoriteOrLiked", "DateLastContentAdded", "SeriesDatePlayed", "ParentIndexNumber", "IndexNumber", "SimilarityScore", "SearchScore", }), path)
    return value
end

function V.check_SortOrder(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Ascending", "Descending", }), path)
    return value
end

function V.check_SubtitleDeliveryMethod(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Encode", "Embed", "External", "Hls", "Drop", }), path)
    return value
end

function V.check_EncodingContext(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Streaming", "Static", }), path)
    return value
end

function V.check_LocationType(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"FileSystem", "Remote", "Virtual", "Offline", }), path)
    return value
end

function V.check_VideoType(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"VideoFile", "Iso", "Dvd", "BluRay", }), path)
    return value
end

function V.check_SeriesStatus(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Continuing", "Ended", "Unreleased", }), path)
    return value
end

function V.check_ImageFormat(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Bmp", "Gif", "Jpg", "Png", "Webp", "Svg", }), path)
    return value
end

function V.check_VirtualFolderInfo(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["CollectionType"] ~= nil then
        V.check_Schema522(J, value["CollectionType"], R.child_path(path, "CollectionType"), state, depth + 1)
    end
    if value["ItemId"] ~= nil then
        V.check_Schema523(J, value["ItemId"], R.child_path(path, "ItemId"), state, depth + 1)
    end
    if value["LibraryOptions"] ~= nil then
        V.check_Schema524(J, value["LibraryOptions"], R.child_path(path, "LibraryOptions"), state, depth + 1)
    end
    if value["Locations"] ~= nil then
        V.check_Schema525(J, value["Locations"], R.child_path(path, "Locations"), state, depth + 1)
    end
    if value["Name"] ~= nil then
        V.check_Schema526(J, value["Name"], R.child_path(path, "Name"), state, depth + 1)
    end
    if value["PrimaryImageItemId"] ~= nil then
        V.check_Schema527(J, value["PrimaryImageItemId"], R.child_path(path, "PrimaryImageItemId"), state, depth + 1)
    end
    if value["RefreshProgress"] ~= nil then
        V.check_Schema528(J, value["RefreshProgress"], R.child_path(path, "RefreshProgress"), state, depth + 1)
    end
    if value["RefreshStatus"] ~= nil then
        V.check_Schema529(J, value["RefreshStatus"], R.child_path(path, "RefreshStatus"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "CollectionType" and key ~= "ItemId" and key ~= "LibraryOptions" and key ~= "Locations" and key ~= "Name" and key ~= "PrimaryImageItemId" and key ~= "RefreshProgress" and key ~= "RefreshStatus" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema522(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.enum(J, value, J.array({"movies", "tvshows", "music", "musicvideos", "homevideos", "boxsets", "books", "mixed", }), path)
    V.check_CollectionTypeOptions(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema523(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema524(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    V.check_LibraryOptions(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema525(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema530(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema526(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema527(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema528(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema529(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema530(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_CollectionType(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"unknown", "movies", "tvshows", "music", "musicvideos", "trailers", "homevideos", "boxsets", "books", "photos", "livetv", "playlists", "folders", }), path)
    return value
end

function V.check_AuthenticateUserByName(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["Pw"] ~= nil then
        V.check_Schema533(J, value["Pw"], R.child_path(path, "Pw"), state, depth + 1)
    end
    if value["Username"] ~= nil then
        V.check_Schema534(J, value["Username"], R.child_path(path, "Username"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "Pw" and key ~= "Username" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema533(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema534(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_LyricLine(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["Start"] ~= nil then
        V.check_Schema536(J, value["Start"], R.child_path(path, "Start"), state, depth + 1)
    end
    if value["Text"] ~= nil then
        V.check_Schema537(J, value["Text"], R.child_path(path, "Text"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "Start" and key ~= "Text" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema536(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema537(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_LyricMetadata(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["Album"] ~= nil then
        V.check_Schema539(J, value["Album"], R.child_path(path, "Album"), state, depth + 1)
    end
    if value["Artist"] ~= nil then
        V.check_Schema540(J, value["Artist"], R.child_path(path, "Artist"), state, depth + 1)
    end
    if value["Author"] ~= nil then
        V.check_Schema541(J, value["Author"], R.child_path(path, "Author"), state, depth + 1)
    end
    if value["By"] ~= nil then
        V.check_Schema542(J, value["By"], R.child_path(path, "By"), state, depth + 1)
    end
    if value["Creator"] ~= nil then
        V.check_Schema543(J, value["Creator"], R.child_path(path, "Creator"), state, depth + 1)
    end
    if value["IsSynced"] ~= nil then
        V.check_Schema544(J, value["IsSynced"], R.child_path(path, "IsSynced"), state, depth + 1)
    end
    if value["Length"] ~= nil then
        V.check_Schema545(J, value["Length"], R.child_path(path, "Length"), state, depth + 1)
    end
    if value["Offset"] ~= nil then
        V.check_Schema546(J, value["Offset"], R.child_path(path, "Offset"), state, depth + 1)
    end
    if value["Title"] ~= nil then
        V.check_Schema547(J, value["Title"], R.child_path(path, "Title"), state, depth + 1)
    end
    if value["Version"] ~= nil then
        V.check_Schema548(J, value["Version"], R.child_path(path, "Version"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "Album" and key ~= "Artist" and key ~= "Author" and key ~= "By" and key ~= "Creator" and key ~= "IsSynced" and key ~= "Length" and key ~= "Offset" and key ~= "Title" and key ~= "Version" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema539(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema540(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema541(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema542(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema543(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema544(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema545(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema546(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema547(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema548(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_DayOfWeek(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", }), path)
    return value
end

function V.check_NameGuidPair(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["Id"] ~= nil then
        V.check_Schema551(J, value["Id"], R.child_path(path, "Id"), state, depth + 1)
    end
    if value["Name"] ~= nil then
        V.check_Schema552(J, value["Name"], R.child_path(path, "Name"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "Id" and key ~= "Name" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema551(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema552(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_ProgramAudio(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Mono", "Stereo", "Dolby", "DolbyDigital", "Thx", "Atmos", }), path)
    return value
end

function V.check_ChannelType(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"TV", "Radio", }), path)
    return value
end

function V.check_ChapterInfo(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["ImageDateModified"] ~= nil then
        V.check_Schema556(J, value["ImageDateModified"], R.child_path(path, "ImageDateModified"), state, depth + 1)
    end
    if value["ImagePath"] ~= nil then
        V.check_Schema557(J, value["ImagePath"], R.child_path(path, "ImagePath"), state, depth + 1)
    end
    if value["ImageTag"] ~= nil then
        V.check_Schema558(J, value["ImageTag"], R.child_path(path, "ImageTag"), state, depth + 1)
    end
    if value["Name"] ~= nil then
        V.check_Schema559(J, value["Name"], R.child_path(path, "Name"), state, depth + 1)
    end
    if value["StartPositionTicks"] ~= nil then
        V.check_Schema560(J, value["StartPositionTicks"], R.child_path(path, "StartPositionTicks"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "ImageDateModified" and key ~= "ImagePath" and key ~= "ImageTag" and key ~= "Name" and key ~= "StartPositionTicks" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema556(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema557(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema558(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema559(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema560(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_ExternalUrl(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["Name"] ~= nil then
        V.check_Schema562(J, value["Name"], R.child_path(path, "Name"), state, depth + 1)
    end
    if value["Url"] ~= nil then
        V.check_Schema563(J, value["Url"], R.child_path(path, "Url"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "Name" and key ~= "Url" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema562(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema563(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_ExtraType(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Unknown", "Clip", "Trailer", "BehindTheScenes", "DeletedScene", "Interview", "Scene", "Sample", "ThemeSong", "ThemeVideo", "Featurette", "Short", }), path)
    return value
end

function V.check_ImageOrientation(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"TopLeft", "TopRight", "BottomRight", "BottomLeft", "LeftTop", "RightTop", "RightBottom", "LeftBottom", }), path)
    return value
end

function V.check_IsoType(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Dvd", "BluRay", }), path)
    return value
end

function V.check_MetadataField(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Cast", "Genres", "ProductionLocations", "Studios", "Tags", "Name", "Overview", "Runtime", "OfficialRating", }), path)
    return value
end

function V.check_MediaSourceInfo(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["AnalyzeDurationMs"] ~= nil then
        V.check_Schema569(J, value["AnalyzeDurationMs"], R.child_path(path, "AnalyzeDurationMs"), state, depth + 1)
    end
    if value["Bitrate"] ~= nil then
        V.check_Schema570(J, value["Bitrate"], R.child_path(path, "Bitrate"), state, depth + 1)
    end
    if value["BufferMs"] ~= nil then
        V.check_Schema571(J, value["BufferMs"], R.child_path(path, "BufferMs"), state, depth + 1)
    end
    if value["Container"] ~= nil then
        V.check_Schema572(J, value["Container"], R.child_path(path, "Container"), state, depth + 1)
    end
    if value["DefaultAudioStreamIndex"] ~= nil then
        V.check_Schema573(J, value["DefaultAudioStreamIndex"], R.child_path(path, "DefaultAudioStreamIndex"), state, depth + 1)
    end
    if value["DefaultSubtitleStreamIndex"] ~= nil then
        V.check_Schema574(J, value["DefaultSubtitleStreamIndex"], R.child_path(path, "DefaultSubtitleStreamIndex"), state, depth + 1)
    end
    if value["ETag"] ~= nil then
        V.check_Schema575(J, value["ETag"], R.child_path(path, "ETag"), state, depth + 1)
    end
    if value["EncoderPath"] ~= nil then
        V.check_Schema576(J, value["EncoderPath"], R.child_path(path, "EncoderPath"), state, depth + 1)
    end
    if value["EncoderProtocol"] ~= nil then
        V.check_Schema577(J, value["EncoderProtocol"], R.child_path(path, "EncoderProtocol"), state, depth + 1)
    end
    if value["FallbackMaxStreamingBitrate"] ~= nil then
        V.check_Schema578(J, value["FallbackMaxStreamingBitrate"], R.child_path(path, "FallbackMaxStreamingBitrate"), state, depth + 1)
    end
    if value["Formats"] ~= nil then
        V.check_Schema579(J, value["Formats"], R.child_path(path, "Formats"), state, depth + 1)
    end
    if value["GenPtsInput"] ~= nil then
        V.check_Schema580(J, value["GenPtsInput"], R.child_path(path, "GenPtsInput"), state, depth + 1)
    end
    if value["HasSegments"] ~= nil then
        V.check_Schema581(J, value["HasSegments"], R.child_path(path, "HasSegments"), state, depth + 1)
    end
    if value["Id"] ~= nil then
        V.check_Schema582(J, value["Id"], R.child_path(path, "Id"), state, depth + 1)
    end
    if value["IgnoreDts"] ~= nil then
        V.check_Schema583(J, value["IgnoreDts"], R.child_path(path, "IgnoreDts"), state, depth + 1)
    end
    if value["IgnoreIndex"] ~= nil then
        V.check_Schema584(J, value["IgnoreIndex"], R.child_path(path, "IgnoreIndex"), state, depth + 1)
    end
    if value["IsInfiniteStream"] ~= nil then
        V.check_Schema585(J, value["IsInfiniteStream"], R.child_path(path, "IsInfiniteStream"), state, depth + 1)
    end
    if value["IsRemote"] ~= nil then
        V.check_Schema586(J, value["IsRemote"], R.child_path(path, "IsRemote"), state, depth + 1)
    end
    if value["IsoType"] ~= nil then
        V.check_Schema587(J, value["IsoType"], R.child_path(path, "IsoType"), state, depth + 1)
    end
    if value["LiveStreamId"] ~= nil then
        V.check_Schema588(J, value["LiveStreamId"], R.child_path(path, "LiveStreamId"), state, depth + 1)
    end
    if value["MediaAttachments"] ~= nil then
        V.check_Schema589(J, value["MediaAttachments"], R.child_path(path, "MediaAttachments"), state, depth + 1)
    end
    if value["MediaStreams"] ~= nil then
        V.check_Schema590(J, value["MediaStreams"], R.child_path(path, "MediaStreams"), state, depth + 1)
    end
    if value["Name"] ~= nil then
        V.check_Schema591(J, value["Name"], R.child_path(path, "Name"), state, depth + 1)
    end
    if value["OpenToken"] ~= nil then
        V.check_Schema592(J, value["OpenToken"], R.child_path(path, "OpenToken"), state, depth + 1)
    end
    if value["Path"] ~= nil then
        V.check_Schema593(J, value["Path"], R.child_path(path, "Path"), state, depth + 1)
    end
    if value["Protocol"] ~= nil then
        V.check_Schema594(J, value["Protocol"], R.child_path(path, "Protocol"), state, depth + 1)
    end
    if value["ReadAtNativeFramerate"] ~= nil then
        V.check_Schema595(J, value["ReadAtNativeFramerate"], R.child_path(path, "ReadAtNativeFramerate"), state, depth + 1)
    end
    if value["RequiredHttpHeaders"] ~= nil then
        V.check_Schema596(J, value["RequiredHttpHeaders"], R.child_path(path, "RequiredHttpHeaders"), state, depth + 1)
    end
    if value["RequiresClosing"] ~= nil then
        V.check_Schema597(J, value["RequiresClosing"], R.child_path(path, "RequiresClosing"), state, depth + 1)
    end
    if value["RequiresLooping"] ~= nil then
        V.check_Schema598(J, value["RequiresLooping"], R.child_path(path, "RequiresLooping"), state, depth + 1)
    end
    if value["RequiresOpening"] ~= nil then
        V.check_Schema599(J, value["RequiresOpening"], R.child_path(path, "RequiresOpening"), state, depth + 1)
    end
    if value["RunTimeTicks"] ~= nil then
        V.check_Schema600(J, value["RunTimeTicks"], R.child_path(path, "RunTimeTicks"), state, depth + 1)
    end
    if value["Size"] ~= nil then
        V.check_Schema601(J, value["Size"], R.child_path(path, "Size"), state, depth + 1)
    end
    if value["SupportsDirectPlay"] ~= nil then
        V.check_Schema602(J, value["SupportsDirectPlay"], R.child_path(path, "SupportsDirectPlay"), state, depth + 1)
    end
    if value["SupportsDirectStream"] ~= nil then
        V.check_Schema603(J, value["SupportsDirectStream"], R.child_path(path, "SupportsDirectStream"), state, depth + 1)
    end
    if value["SupportsProbing"] ~= nil then
        V.check_Schema604(J, value["SupportsProbing"], R.child_path(path, "SupportsProbing"), state, depth + 1)
    end
    if value["SupportsTranscoding"] ~= nil then
        V.check_Schema605(J, value["SupportsTranscoding"], R.child_path(path, "SupportsTranscoding"), state, depth + 1)
    end
    if value["Timestamp"] ~= nil then
        V.check_Schema606(J, value["Timestamp"], R.child_path(path, "Timestamp"), state, depth + 1)
    end
    if value["TranscodingContainer"] ~= nil then
        V.check_Schema607(J, value["TranscodingContainer"], R.child_path(path, "TranscodingContainer"), state, depth + 1)
    end
    if value["TranscodingSubProtocol"] ~= nil then
        V.check_Schema608(J, value["TranscodingSubProtocol"], R.child_path(path, "TranscodingSubProtocol"), state, depth + 1)
    end
    if value["TranscodingUrl"] ~= nil then
        V.check_Schema609(J, value["TranscodingUrl"], R.child_path(path, "TranscodingUrl"), state, depth + 1)
    end
    if value["Type"] ~= nil then
        V.check_Schema610(J, value["Type"], R.child_path(path, "Type"), state, depth + 1)
    end
    if value["UseMostCompatibleTranscodingProfile"] ~= nil then
        V.check_Schema611(J, value["UseMostCompatibleTranscodingProfile"], R.child_path(path, "UseMostCompatibleTranscodingProfile"), state, depth + 1)
    end
    if value["Video3DFormat"] ~= nil then
        V.check_Schema612(J, value["Video3DFormat"], R.child_path(path, "Video3DFormat"), state, depth + 1)
    end
    if value["VideoType"] ~= nil then
        V.check_Schema613(J, value["VideoType"], R.child_path(path, "VideoType"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "AnalyzeDurationMs" and key ~= "Bitrate" and key ~= "BufferMs" and key ~= "Container" and key ~= "DefaultAudioStreamIndex" and key ~= "DefaultSubtitleStreamIndex" and key ~= "ETag" and key ~= "EncoderPath" and key ~= "EncoderProtocol" and key ~= "FallbackMaxStreamingBitrate" and key ~= "Formats" and key ~= "GenPtsInput" and key ~= "HasSegments" and key ~= "Id" and key ~= "IgnoreDts" and key ~= "IgnoreIndex" and key ~= "IsInfiniteStream" and key ~= "IsRemote" and key ~= "IsoType" and key ~= "LiveStreamId" and key ~= "MediaAttachments" and key ~= "MediaStreams" and key ~= "Name" and key ~= "OpenToken" and key ~= "Path" and key ~= "Protocol" and key ~= "ReadAtNativeFramerate" and key ~= "RequiredHttpHeaders" and key ~= "RequiresClosing" and key ~= "RequiresLooping" and key ~= "RequiresOpening" and key ~= "RunTimeTicks" and key ~= "Size" and key ~= "SupportsDirectPlay" and key ~= "SupportsDirectStream" and key ~= "SupportsProbing" and key ~= "SupportsTranscoding" and key ~= "Timestamp" and key ~= "TranscodingContainer" and key ~= "TranscodingSubProtocol" and key ~= "TranscodingUrl" and key ~= "Type" and key ~= "UseMostCompatibleTranscodingProfile" and key ~= "Video3DFormat" and key ~= "VideoType" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema569(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema570(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema571(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema572(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema573(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema574(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema575(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema576(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema577(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.enum(J, value, J.array({"File", "Http", "Rtmp", "Rtsp", "Udp", "Rtp", "Ftp", }), path)
    V.check_MediaProtocol(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema578(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema579(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema614(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema580(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema581(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema582(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema583(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema584(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema585(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema586(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema587(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.enum(J, value, J.array({"Dvd", "BluRay", }), path)
    V.check_IsoType(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema588(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema589(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_MediaAttachment(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema590(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_MediaStream(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema591(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema592(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema593(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema594(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"File", "Http", "Rtmp", "Rtsp", "Udp", "Rtp", "Ftp", }), path)
    V.check_MediaProtocol(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema595(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema596(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema615(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema597(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema598(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema599(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema600(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema601(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema602(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema603(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema604(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema605(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema606(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.enum(J, value, J.array({"None", "Zero", "Valid", }), path)
    V.check_TransportStreamTimestamp(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema607(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema608(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"http", "hls", }), path)
    V.check_MediaStreamProtocol(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema609(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema610(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"Default", "Grouping", "Placeholder", }), path)
    V.check_MediaSourceType(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema611(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema612(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.enum(J, value, J.array({"HalfSideBySide", "FullSideBySide", "FullTopAndBottom", "HalfTopAndBottom", "MVC", }), path)
    V.check_Video3DFormat(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema613(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.enum(J, value, J.array({"VideoFile", "Iso", "Dvd", "BluRay", }), path)
    V.check_VideoType(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema614(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema615(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_MediaStream(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["AspectRatio"] ~= nil then
        V.check_Schema617(J, value["AspectRatio"], R.child_path(path, "AspectRatio"), state, depth + 1)
    end
    if value["AudioSpatialFormat"] ~= nil then
        V.check_Schema618(J, value["AudioSpatialFormat"], R.child_path(path, "AudioSpatialFormat"), state, depth + 1)
    end
    if value["AverageFrameRate"] ~= nil then
        V.check_Schema619(J, value["AverageFrameRate"], R.child_path(path, "AverageFrameRate"), state, depth + 1)
    end
    if value["BitDepth"] ~= nil then
        V.check_Schema620(J, value["BitDepth"], R.child_path(path, "BitDepth"), state, depth + 1)
    end
    if value["BitRate"] ~= nil then
        V.check_Schema621(J, value["BitRate"], R.child_path(path, "BitRate"), state, depth + 1)
    end
    if value["BlPresentFlag"] ~= nil then
        V.check_Schema622(J, value["BlPresentFlag"], R.child_path(path, "BlPresentFlag"), state, depth + 1)
    end
    if value["ChannelLayout"] ~= nil then
        V.check_Schema623(J, value["ChannelLayout"], R.child_path(path, "ChannelLayout"), state, depth + 1)
    end
    if value["Channels"] ~= nil then
        V.check_Schema624(J, value["Channels"], R.child_path(path, "Channels"), state, depth + 1)
    end
    if value["Codec"] ~= nil then
        V.check_Schema625(J, value["Codec"], R.child_path(path, "Codec"), state, depth + 1)
    end
    if value["CodecTag"] ~= nil then
        V.check_Schema626(J, value["CodecTag"], R.child_path(path, "CodecTag"), state, depth + 1)
    end
    if value["CodecTimeBase"] ~= nil then
        V.check_Schema627(J, value["CodecTimeBase"], R.child_path(path, "CodecTimeBase"), state, depth + 1)
    end
    if value["ColorPrimaries"] ~= nil then
        V.check_Schema628(J, value["ColorPrimaries"], R.child_path(path, "ColorPrimaries"), state, depth + 1)
    end
    if value["ColorRange"] ~= nil then
        V.check_Schema629(J, value["ColorRange"], R.child_path(path, "ColorRange"), state, depth + 1)
    end
    if value["ColorSpace"] ~= nil then
        V.check_Schema630(J, value["ColorSpace"], R.child_path(path, "ColorSpace"), state, depth + 1)
    end
    if value["ColorTransfer"] ~= nil then
        V.check_Schema631(J, value["ColorTransfer"], R.child_path(path, "ColorTransfer"), state, depth + 1)
    end
    if value["Comment"] ~= nil then
        V.check_Schema632(J, value["Comment"], R.child_path(path, "Comment"), state, depth + 1)
    end
    if value["DeliveryMethod"] ~= nil then
        V.check_Schema633(J, value["DeliveryMethod"], R.child_path(path, "DeliveryMethod"), state, depth + 1)
    end
    if value["DeliveryUrl"] ~= nil then
        V.check_Schema634(J, value["DeliveryUrl"], R.child_path(path, "DeliveryUrl"), state, depth + 1)
    end
    if value["DisplayTitle"] ~= nil then
        V.check_Schema635(J, value["DisplayTitle"], R.child_path(path, "DisplayTitle"), state, depth + 1)
    end
    if value["DvBlSignalCompatibilityId"] ~= nil then
        V.check_Schema636(J, value["DvBlSignalCompatibilityId"], R.child_path(path, "DvBlSignalCompatibilityId"), state, depth + 1)
    end
    if value["DvLevel"] ~= nil then
        V.check_Schema637(J, value["DvLevel"], R.child_path(path, "DvLevel"), state, depth + 1)
    end
    if value["DvProfile"] ~= nil then
        V.check_Schema638(J, value["DvProfile"], R.child_path(path, "DvProfile"), state, depth + 1)
    end
    if value["DvVersionMajor"] ~= nil then
        V.check_Schema639(J, value["DvVersionMajor"], R.child_path(path, "DvVersionMajor"), state, depth + 1)
    end
    if value["DvVersionMinor"] ~= nil then
        V.check_Schema640(J, value["DvVersionMinor"], R.child_path(path, "DvVersionMinor"), state, depth + 1)
    end
    if value["ElPresentFlag"] ~= nil then
        V.check_Schema641(J, value["ElPresentFlag"], R.child_path(path, "ElPresentFlag"), state, depth + 1)
    end
    if value["Height"] ~= nil then
        V.check_Schema642(J, value["Height"], R.child_path(path, "Height"), state, depth + 1)
    end
    if value["Index"] ~= nil then
        V.check_Schema643(J, value["Index"], R.child_path(path, "Index"), state, depth + 1)
    end
    if value["IsAVC"] ~= nil then
        V.check_Schema644(J, value["IsAVC"], R.child_path(path, "IsAVC"), state, depth + 1)
    end
    if value["IsAnamorphic"] ~= nil then
        V.check_Schema645(J, value["IsAnamorphic"], R.child_path(path, "IsAnamorphic"), state, depth + 1)
    end
    if value["IsDefault"] ~= nil then
        V.check_Schema646(J, value["IsDefault"], R.child_path(path, "IsDefault"), state, depth + 1)
    end
    if value["IsExternal"] ~= nil then
        V.check_Schema647(J, value["IsExternal"], R.child_path(path, "IsExternal"), state, depth + 1)
    end
    if value["IsExternalUrl"] ~= nil then
        V.check_Schema648(J, value["IsExternalUrl"], R.child_path(path, "IsExternalUrl"), state, depth + 1)
    end
    if value["IsForced"] ~= nil then
        V.check_Schema649(J, value["IsForced"], R.child_path(path, "IsForced"), state, depth + 1)
    end
    if value["IsHearingImpaired"] ~= nil then
        V.check_Schema650(J, value["IsHearingImpaired"], R.child_path(path, "IsHearingImpaired"), state, depth + 1)
    end
    if value["IsInterlaced"] ~= nil then
        V.check_Schema651(J, value["IsInterlaced"], R.child_path(path, "IsInterlaced"), state, depth + 1)
    end
    if value["IsTextSubtitleStream"] ~= nil then
        V.check_Schema652(J, value["IsTextSubtitleStream"], R.child_path(path, "IsTextSubtitleStream"), state, depth + 1)
    end
    if value["Language"] ~= nil then
        V.check_Schema653(J, value["Language"], R.child_path(path, "Language"), state, depth + 1)
    end
    if value["Level"] ~= nil then
        V.check_Schema654(J, value["Level"], R.child_path(path, "Level"), state, depth + 1)
    end
    if value["LocalizedDefault"] ~= nil then
        V.check_Schema655(J, value["LocalizedDefault"], R.child_path(path, "LocalizedDefault"), state, depth + 1)
    end
    if value["LocalizedExternal"] ~= nil then
        V.check_Schema656(J, value["LocalizedExternal"], R.child_path(path, "LocalizedExternal"), state, depth + 1)
    end
    if value["LocalizedForced"] ~= nil then
        V.check_Schema657(J, value["LocalizedForced"], R.child_path(path, "LocalizedForced"), state, depth + 1)
    end
    if value["LocalizedHearingImpaired"] ~= nil then
        V.check_Schema658(J, value["LocalizedHearingImpaired"], R.child_path(path, "LocalizedHearingImpaired"), state, depth + 1)
    end
    if value["LocalizedUndefined"] ~= nil then
        V.check_Schema659(J, value["LocalizedUndefined"], R.child_path(path, "LocalizedUndefined"), state, depth + 1)
    end
    if value["NalLengthSize"] ~= nil then
        V.check_Schema660(J, value["NalLengthSize"], R.child_path(path, "NalLengthSize"), state, depth + 1)
    end
    if value["PacketLength"] ~= nil then
        V.check_Schema661(J, value["PacketLength"], R.child_path(path, "PacketLength"), state, depth + 1)
    end
    if value["Path"] ~= nil then
        V.check_Schema662(J, value["Path"], R.child_path(path, "Path"), state, depth + 1)
    end
    if value["PixelFormat"] ~= nil then
        V.check_Schema663(J, value["PixelFormat"], R.child_path(path, "PixelFormat"), state, depth + 1)
    end
    if value["Profile"] ~= nil then
        V.check_Schema664(J, value["Profile"], R.child_path(path, "Profile"), state, depth + 1)
    end
    if value["RealFrameRate"] ~= nil then
        V.check_Schema665(J, value["RealFrameRate"], R.child_path(path, "RealFrameRate"), state, depth + 1)
    end
    if value["RefFrames"] ~= nil then
        V.check_Schema666(J, value["RefFrames"], R.child_path(path, "RefFrames"), state, depth + 1)
    end
    if value["ReferenceFrameRate"] ~= nil then
        V.check_Schema667(J, value["ReferenceFrameRate"], R.child_path(path, "ReferenceFrameRate"), state, depth + 1)
    end
    if value["Rotation"] ~= nil then
        V.check_Schema668(J, value["Rotation"], R.child_path(path, "Rotation"), state, depth + 1)
    end
    if value["RpuPresentFlag"] ~= nil then
        V.check_Schema669(J, value["RpuPresentFlag"], R.child_path(path, "RpuPresentFlag"), state, depth + 1)
    end
    if value["SampleRate"] ~= nil then
        V.check_Schema670(J, value["SampleRate"], R.child_path(path, "SampleRate"), state, depth + 1)
    end
    if value["Score"] ~= nil then
        V.check_Schema671(J, value["Score"], R.child_path(path, "Score"), state, depth + 1)
    end
    if value["SupportsExternalStream"] ~= nil then
        V.check_Schema672(J, value["SupportsExternalStream"], R.child_path(path, "SupportsExternalStream"), state, depth + 1)
    end
    if value["TimeBase"] ~= nil then
        V.check_Schema673(J, value["TimeBase"], R.child_path(path, "TimeBase"), state, depth + 1)
    end
    if value["Title"] ~= nil then
        V.check_Schema674(J, value["Title"], R.child_path(path, "Title"), state, depth + 1)
    end
    if value["Type"] ~= nil then
        V.check_Schema675(J, value["Type"], R.child_path(path, "Type"), state, depth + 1)
    end
    if value["VideoDoViTitle"] ~= nil then
        V.check_Schema676(J, value["VideoDoViTitle"], R.child_path(path, "VideoDoViTitle"), state, depth + 1)
    end
    if value["VideoRange"] ~= nil then
        V.check_Schema677(J, value["VideoRange"], R.child_path(path, "VideoRange"), state, depth + 1)
    end
    if value["VideoRangeType"] ~= nil then
        V.check_Schema678(J, value["VideoRangeType"], R.child_path(path, "VideoRangeType"), state, depth + 1)
    end
    if value["Width"] ~= nil then
        V.check_Schema679(J, value["Width"], R.child_path(path, "Width"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "AspectRatio" and key ~= "AudioSpatialFormat" and key ~= "AverageFrameRate" and key ~= "BitDepth" and key ~= "BitRate" and key ~= "BlPresentFlag" and key ~= "ChannelLayout" and key ~= "Channels" and key ~= "Codec" and key ~= "CodecTag" and key ~= "CodecTimeBase" and key ~= "ColorPrimaries" and key ~= "ColorRange" and key ~= "ColorSpace" and key ~= "ColorTransfer" and key ~= "Comment" and key ~= "DeliveryMethod" and key ~= "DeliveryUrl" and key ~= "DisplayTitle" and key ~= "DvBlSignalCompatibilityId" and key ~= "DvLevel" and key ~= "DvProfile" and key ~= "DvVersionMajor" and key ~= "DvVersionMinor" and key ~= "ElPresentFlag" and key ~= "Height" and key ~= "Index" and key ~= "IsAVC" and key ~= "IsAnamorphic" and key ~= "IsDefault" and key ~= "IsExternal" and key ~= "IsExternalUrl" and key ~= "IsForced" and key ~= "IsHearingImpaired" and key ~= "IsInterlaced" and key ~= "IsTextSubtitleStream" and key ~= "Language" and key ~= "Level" and key ~= "LocalizedDefault" and key ~= "LocalizedExternal" and key ~= "LocalizedForced" and key ~= "LocalizedHearingImpaired" and key ~= "LocalizedUndefined" and key ~= "NalLengthSize" and key ~= "PacketLength" and key ~= "Path" and key ~= "PixelFormat" and key ~= "Profile" and key ~= "RealFrameRate" and key ~= "RefFrames" and key ~= "ReferenceFrameRate" and key ~= "Rotation" and key ~= "RpuPresentFlag" and key ~= "SampleRate" and key ~= "Score" and key ~= "SupportsExternalStream" and key ~= "TimeBase" and key ~= "Title" and key ~= "Type" and key ~= "VideoDoViTitle" and key ~= "VideoRange" and key ~= "VideoRangeType" and key ~= "Width" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema617(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema618(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"None", "DolbyAtmos", "DTSX", }), path)
    V.check_AudioSpatialFormat(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema619(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema620(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema621(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema622(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema623(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema624(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema625(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema626(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema627(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema628(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema629(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema630(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema631(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema632(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema633(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.enum(J, value, J.array({"Encode", "Embed", "External", "Hls", "Drop", }), path)
    V.check_SubtitleDeliveryMethod(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema634(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema635(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema636(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema637(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema638(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema639(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema640(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema641(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema642(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema643(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema644(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema645(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema646(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema647(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema648(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema649(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema650(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema651(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema652(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema653(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema654(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema655(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema656(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema657(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema658(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema659(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema660(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema661(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema662(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema663(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema664(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema665(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema666(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema667(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema668(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema669(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema670(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema671(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema672(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema673(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema674(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema675(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"Audio", "Video", "Subtitle", "EmbeddedImage", "Data", "Lyric", }), path)
    V.check_MediaStreamType(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema676(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema677(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"Unknown", "SDR", "HDR", }), path)
    V.check_VideoRange(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema678(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"Unknown", "SDR", "HDR10", "HLG", "DOVI", "DOVIWithHDR10", "DOVIWithHLG", "DOVIWithSDR", "HDR10Plus", }), path)
    V.check_VideoRangeType(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema679(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_BaseItemPerson(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["Id"] ~= nil then
        V.check_Schema681(J, value["Id"], R.child_path(path, "Id"), state, depth + 1)
    end
    if value["ImageBlurHashes"] ~= nil then
        V.check_Schema682(J, value["ImageBlurHashes"], R.child_path(path, "ImageBlurHashes"), state, depth + 1)
    end
    if value["Name"] ~= nil then
        V.check_Schema683(J, value["Name"], R.child_path(path, "Name"), state, depth + 1)
    end
    if value["PrimaryImageTag"] ~= nil then
        V.check_Schema684(J, value["PrimaryImageTag"], R.child_path(path, "PrimaryImageTag"), state, depth + 1)
    end
    if value["Role"] ~= nil then
        V.check_Schema685(J, value["Role"], R.child_path(path, "Role"), state, depth + 1)
    end
    if value["Type"] ~= nil then
        V.check_Schema686(J, value["Type"], R.child_path(path, "Type"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "Id" and key ~= "ImageBlurHashes" and key ~= "Name" and key ~= "PrimaryImageTag" and key ~= "Role" and key ~= "Type" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema681(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema682(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.object(J, value, path)
    if value["Art"] ~= nil then
        V.check_Schema687(J, value["Art"], R.child_path(path, "Art"), state, depth + 1)
    end
    if value["Backdrop"] ~= nil then
        V.check_Schema688(J, value["Backdrop"], R.child_path(path, "Backdrop"), state, depth + 1)
    end
    if value["Banner"] ~= nil then
        V.check_Schema689(J, value["Banner"], R.child_path(path, "Banner"), state, depth + 1)
    end
    if value["Box"] ~= nil then
        V.check_Schema690(J, value["Box"], R.child_path(path, "Box"), state, depth + 1)
    end
    if value["BoxRear"] ~= nil then
        V.check_Schema691(J, value["BoxRear"], R.child_path(path, "BoxRear"), state, depth + 1)
    end
    if value["Chapter"] ~= nil then
        V.check_Schema692(J, value["Chapter"], R.child_path(path, "Chapter"), state, depth + 1)
    end
    if value["Disc"] ~= nil then
        V.check_Schema693(J, value["Disc"], R.child_path(path, "Disc"), state, depth + 1)
    end
    if value["Logo"] ~= nil then
        V.check_Schema694(J, value["Logo"], R.child_path(path, "Logo"), state, depth + 1)
    end
    if value["Menu"] ~= nil then
        V.check_Schema695(J, value["Menu"], R.child_path(path, "Menu"), state, depth + 1)
    end
    if value["Primary"] ~= nil then
        V.check_Schema696(J, value["Primary"], R.child_path(path, "Primary"), state, depth + 1)
    end
    if value["Profile"] ~= nil then
        V.check_Schema697(J, value["Profile"], R.child_path(path, "Profile"), state, depth + 1)
    end
    if value["Screenshot"] ~= nil then
        V.check_Schema698(J, value["Screenshot"], R.child_path(path, "Screenshot"), state, depth + 1)
    end
    if value["Thumb"] ~= nil then
        V.check_Schema699(J, value["Thumb"], R.child_path(path, "Thumb"), state, depth + 1)
    end
    return value
end

function V.check_Schema683(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema684(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema685(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema686(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"Unknown", "Actor", "Director", "Composer", "Writer", "GuestStar", "Producer", "Conductor", "Lyricist", "Arranger", "Engineer", "Mixer", "Remixer", "Creator", "Artist", "AlbumArtist", "Author", "Illustrator", "Penciller", "Inker", "Colorist", "Letterer", "CoverArtist", "Editor", "Translator", }), path)
    V.check_PersonKind(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema687(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema700(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema688(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema701(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema689(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema702(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema690(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema703(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema691(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema704(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema692(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema705(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema693(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema706(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema694(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema707(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema695(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema708(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema696(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema709(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema697(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema710(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema698(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema711(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema699(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    for key, child in pairs(value) do
        if true then
            V.check_Schema712(J, child, R.child_path(path, key), state, depth + 1)
        end
    end
    return value
end

function V.check_Schema700(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema701(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema702(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema703(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema704(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema705(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema706(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema707(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema708(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema709(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema710(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema711(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema712(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_PlayAccess(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Full", "None", }), path)
    return value
end

function V.check_MediaUrl(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["Name"] ~= nil then
        V.check_Schema715(J, value["Name"], R.child_path(path, "Name"), state, depth + 1)
    end
    if value["Url"] ~= nil then
        V.check_Schema716(J, value["Url"], R.child_path(path, "Url"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "Name" and key ~= "Url" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema715(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema716(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Video3DFormat(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"HalfSideBySide", "FullSideBySide", "FullTopAndBottom", "HalfTopAndBottom", "MVC", }), path)
    return value
end

function V.check_TrickplayInfo(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["Bandwidth"] ~= nil then
        V.check_Schema719(J, value["Bandwidth"], R.child_path(path, "Bandwidth"), state, depth + 1)
    end
    if value["Height"] ~= nil then
        V.check_Schema720(J, value["Height"], R.child_path(path, "Height"), state, depth + 1)
    end
    if value["Interval"] ~= nil then
        V.check_Schema721(J, value["Interval"], R.child_path(path, "Interval"), state, depth + 1)
    end
    if value["ThumbnailCount"] ~= nil then
        V.check_Schema722(J, value["ThumbnailCount"], R.child_path(path, "ThumbnailCount"), state, depth + 1)
    end
    if value["TileHeight"] ~= nil then
        V.check_Schema723(J, value["TileHeight"], R.child_path(path, "TileHeight"), state, depth + 1)
    end
    if value["TileWidth"] ~= nil then
        V.check_Schema724(J, value["TileWidth"], R.child_path(path, "TileWidth"), state, depth + 1)
    end
    if value["Width"] ~= nil then
        V.check_Schema725(J, value["Width"], R.child_path(path, "Width"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "Bandwidth" and key ~= "Height" and key ~= "Interval" and key ~= "ThumbnailCount" and key ~= "TileHeight" and key ~= "TileWidth" and key ~= "Width" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema719(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema720(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema721(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema722(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema723(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema724(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema725(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_SessionInfoDto(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["AdditionalUsers"] ~= nil then
        V.check_Schema727(J, value["AdditionalUsers"], R.child_path(path, "AdditionalUsers"), state, depth + 1)
    end
    if value["ApplicationVersion"] ~= nil then
        V.check_Schema728(J, value["ApplicationVersion"], R.child_path(path, "ApplicationVersion"), state, depth + 1)
    end
    if value["Capabilities"] ~= nil then
        V.check_Schema729(J, value["Capabilities"], R.child_path(path, "Capabilities"), state, depth + 1)
    end
    if value["Client"] ~= nil then
        V.check_Schema730(J, value["Client"], R.child_path(path, "Client"), state, depth + 1)
    end
    if value["DeviceId"] ~= nil then
        V.check_Schema731(J, value["DeviceId"], R.child_path(path, "DeviceId"), state, depth + 1)
    end
    if value["DeviceName"] ~= nil then
        V.check_Schema732(J, value["DeviceName"], R.child_path(path, "DeviceName"), state, depth + 1)
    end
    if value["DeviceType"] ~= nil then
        V.check_Schema733(J, value["DeviceType"], R.child_path(path, "DeviceType"), state, depth + 1)
    end
    if value["HasCustomDeviceName"] ~= nil then
        V.check_Schema734(J, value["HasCustomDeviceName"], R.child_path(path, "HasCustomDeviceName"), state, depth + 1)
    end
    if value["Id"] ~= nil then
        V.check_Schema735(J, value["Id"], R.child_path(path, "Id"), state, depth + 1)
    end
    if value["IsActive"] ~= nil then
        V.check_Schema736(J, value["IsActive"], R.child_path(path, "IsActive"), state, depth + 1)
    end
    if value["LastActivityDate"] ~= nil then
        V.check_Schema737(J, value["LastActivityDate"], R.child_path(path, "LastActivityDate"), state, depth + 1)
    end
    if value["LastPausedDate"] ~= nil then
        V.check_Schema738(J, value["LastPausedDate"], R.child_path(path, "LastPausedDate"), state, depth + 1)
    end
    if value["LastPlaybackCheckIn"] ~= nil then
        V.check_Schema739(J, value["LastPlaybackCheckIn"], R.child_path(path, "LastPlaybackCheckIn"), state, depth + 1)
    end
    if value["NowPlayingItem"] ~= nil then
        V.check_Schema740(J, value["NowPlayingItem"], R.child_path(path, "NowPlayingItem"), state, depth + 1)
    end
    if value["NowPlayingQueue"] ~= nil then
        V.check_Schema741(J, value["NowPlayingQueue"], R.child_path(path, "NowPlayingQueue"), state, depth + 1)
    end
    if value["NowPlayingQueueFullItems"] ~= nil then
        V.check_Schema742(J, value["NowPlayingQueueFullItems"], R.child_path(path, "NowPlayingQueueFullItems"), state, depth + 1)
    end
    if value["NowViewingItem"] ~= nil then
        V.check_Schema743(J, value["NowViewingItem"], R.child_path(path, "NowViewingItem"), state, depth + 1)
    end
    if value["PlayState"] ~= nil then
        V.check_Schema744(J, value["PlayState"], R.child_path(path, "PlayState"), state, depth + 1)
    end
    if value["PlayableMediaTypes"] ~= nil then
        V.check_Schema745(J, value["PlayableMediaTypes"], R.child_path(path, "PlayableMediaTypes"), state, depth + 1)
    end
    if value["PlaylistItemId"] ~= nil then
        V.check_Schema746(J, value["PlaylistItemId"], R.child_path(path, "PlaylistItemId"), state, depth + 1)
    end
    if value["RemoteEndPoint"] ~= nil then
        V.check_Schema747(J, value["RemoteEndPoint"], R.child_path(path, "RemoteEndPoint"), state, depth + 1)
    end
    if value["ServerId"] ~= nil then
        V.check_Schema748(J, value["ServerId"], R.child_path(path, "ServerId"), state, depth + 1)
    end
    if value["SupportedCommands"] ~= nil then
        V.check_Schema749(J, value["SupportedCommands"], R.child_path(path, "SupportedCommands"), state, depth + 1)
    end
    if value["SupportsMediaControl"] ~= nil then
        V.check_Schema750(J, value["SupportsMediaControl"], R.child_path(path, "SupportsMediaControl"), state, depth + 1)
    end
    if value["SupportsRemoteControl"] ~= nil then
        V.check_Schema751(J, value["SupportsRemoteControl"], R.child_path(path, "SupportsRemoteControl"), state, depth + 1)
    end
    if value["TranscodingInfo"] ~= nil then
        V.check_Schema752(J, value["TranscodingInfo"], R.child_path(path, "TranscodingInfo"), state, depth + 1)
    end
    if value["UserId"] ~= nil then
        V.check_Schema753(J, value["UserId"], R.child_path(path, "UserId"), state, depth + 1)
    end
    if value["UserName"] ~= nil then
        V.check_Schema754(J, value["UserName"], R.child_path(path, "UserName"), state, depth + 1)
    end
    if value["UserPrimaryImageTag"] ~= nil then
        V.check_Schema755(J, value["UserPrimaryImageTag"], R.child_path(path, "UserPrimaryImageTag"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "AdditionalUsers" and key ~= "ApplicationVersion" and key ~= "Capabilities" and key ~= "Client" and key ~= "DeviceId" and key ~= "DeviceName" and key ~= "DeviceType" and key ~= "HasCustomDeviceName" and key ~= "Id" and key ~= "IsActive" and key ~= "LastActivityDate" and key ~= "LastPausedDate" and key ~= "LastPlaybackCheckIn" and key ~= "NowPlayingItem" and key ~= "NowPlayingQueue" and key ~= "NowPlayingQueueFullItems" and key ~= "NowViewingItem" and key ~= "PlayState" and key ~= "PlayableMediaTypes" and key ~= "PlaylistItemId" and key ~= "RemoteEndPoint" and key ~= "ServerId" and key ~= "SupportedCommands" and key ~= "SupportsMediaControl" and key ~= "SupportsRemoteControl" and key ~= "TranscodingInfo" and key ~= "UserId" and key ~= "UserName" and key ~= "UserPrimaryImageTag" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema727(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_SessionUserInfo(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema728(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema729(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    V.check_ClientCapabilitiesDto(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema730(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema731(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema732(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema733(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema734(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema735(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema736(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema737(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema738(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema739(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema740(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    V.check_BaseItemDto(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema741(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_QueueItem(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema742(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_BaseItemDto(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema743(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    V.check_BaseItemDto(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema744(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    V.check_PlayerStateInfo(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema745(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_MediaType(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema746(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema747(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema748(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema749(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_GeneralCommandType(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema750(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema751(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema752(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    V.check_TranscodingInfo(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema753(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema754(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema755(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_UserDto(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["Configuration"] ~= nil then
        V.check_Schema757(J, value["Configuration"], R.child_path(path, "Configuration"), state, depth + 1)
    end
    if value["EnableAutoLogin"] ~= nil then
        V.check_Schema758(J, value["EnableAutoLogin"], R.child_path(path, "EnableAutoLogin"), state, depth + 1)
    end
    if value["HasConfiguredEasyPassword"] ~= nil then
        V.check_Schema759(J, value["HasConfiguredEasyPassword"], R.child_path(path, "HasConfiguredEasyPassword"), state, depth + 1)
    end
    if value["HasConfiguredPassword"] ~= nil then
        V.check_Schema760(J, value["HasConfiguredPassword"], R.child_path(path, "HasConfiguredPassword"), state, depth + 1)
    end
    if value["HasPassword"] ~= nil then
        V.check_Schema761(J, value["HasPassword"], R.child_path(path, "HasPassword"), state, depth + 1)
    end
    if value["Id"] ~= nil then
        V.check_Schema762(J, value["Id"], R.child_path(path, "Id"), state, depth + 1)
    end
    if value["LastActivityDate"] ~= nil then
        V.check_Schema763(J, value["LastActivityDate"], R.child_path(path, "LastActivityDate"), state, depth + 1)
    end
    if value["LastLoginDate"] ~= nil then
        V.check_Schema764(J, value["LastLoginDate"], R.child_path(path, "LastLoginDate"), state, depth + 1)
    end
    if value["Name"] ~= nil then
        V.check_Schema765(J, value["Name"], R.child_path(path, "Name"), state, depth + 1)
    end
    if value["Policy"] ~= nil then
        V.check_Schema766(J, value["Policy"], R.child_path(path, "Policy"), state, depth + 1)
    end
    if value["PrimaryImageAspectRatio"] ~= nil then
        V.check_Schema767(J, value["PrimaryImageAspectRatio"], R.child_path(path, "PrimaryImageAspectRatio"), state, depth + 1)
    end
    if value["PrimaryImageTag"] ~= nil then
        V.check_Schema768(J, value["PrimaryImageTag"], R.child_path(path, "PrimaryImageTag"), state, depth + 1)
    end
    if value["ServerId"] ~= nil then
        V.check_Schema769(J, value["ServerId"], R.child_path(path, "ServerId"), state, depth + 1)
    end
    if value["ServerName"] ~= nil then
        V.check_Schema770(J, value["ServerName"], R.child_path(path, "ServerName"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "Configuration" and key ~= "EnableAutoLogin" and key ~= "HasConfiguredEasyPassword" and key ~= "HasConfiguredPassword" and key ~= "HasPassword" and key ~= "Id" and key ~= "LastActivityDate" and key ~= "LastLoginDate" and key ~= "Name" and key ~= "Policy" and key ~= "PrimaryImageAspectRatio" and key ~= "PrimaryImageTag" and key ~= "ServerId" and key ~= "ServerName" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema757(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    V.check_UserConfiguration(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema758(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema759(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema760(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema761(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema762(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema763(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema764(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema765(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema766(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    V.check_UserPolicy(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema767(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema768(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema769(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema770(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_CollectionTypeOptions(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"movies", "tvshows", "music", "musicvideos", "homevideos", "boxsets", "books", "mixed", }), path)
    return value
end

function V.check_LibraryOptions(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["AllowEmbeddedSubtitles"] ~= nil then
        V.check_Schema773(J, value["AllowEmbeddedSubtitles"], R.child_path(path, "AllowEmbeddedSubtitles"), state, depth + 1)
    end
    if value["AutomaticRefreshIntervalDays"] ~= nil then
        V.check_Schema774(J, value["AutomaticRefreshIntervalDays"], R.child_path(path, "AutomaticRefreshIntervalDays"), state, depth + 1)
    end
    if value["AutomaticallyAddToCollection"] ~= nil then
        V.check_Schema775(J, value["AutomaticallyAddToCollection"], R.child_path(path, "AutomaticallyAddToCollection"), state, depth + 1)
    end
    if value["CustomTagDelimiters"] ~= nil then
        V.check_Schema776(J, value["CustomTagDelimiters"], R.child_path(path, "CustomTagDelimiters"), state, depth + 1)
    end
    if value["DelimiterWhitelist"] ~= nil then
        V.check_Schema777(J, value["DelimiterWhitelist"], R.child_path(path, "DelimiterWhitelist"), state, depth + 1)
    end
    if value["DisabledLocalMetadataReaders"] ~= nil then
        V.check_Schema778(J, value["DisabledLocalMetadataReaders"], R.child_path(path, "DisabledLocalMetadataReaders"), state, depth + 1)
    end
    if value["DisabledLyricFetchers"] ~= nil then
        V.check_Schema779(J, value["DisabledLyricFetchers"], R.child_path(path, "DisabledLyricFetchers"), state, depth + 1)
    end
    if value["DisabledMediaSegmentProviders"] ~= nil then
        V.check_Schema780(J, value["DisabledMediaSegmentProviders"], R.child_path(path, "DisabledMediaSegmentProviders"), state, depth + 1)
    end
    if value["DisabledSubtitleFetchers"] ~= nil then
        V.check_Schema781(J, value["DisabledSubtitleFetchers"], R.child_path(path, "DisabledSubtitleFetchers"), state, depth + 1)
    end
    if value["EnableAutomaticSeriesGrouping"] ~= nil then
        V.check_Schema782(J, value["EnableAutomaticSeriesGrouping"], R.child_path(path, "EnableAutomaticSeriesGrouping"), state, depth + 1)
    end
    if value["EnableChapterImageExtraction"] ~= nil then
        V.check_Schema783(J, value["EnableChapterImageExtraction"], R.child_path(path, "EnableChapterImageExtraction"), state, depth + 1)
    end
    if value["EnableEmbeddedEpisodeInfos"] ~= nil then
        V.check_Schema784(J, value["EnableEmbeddedEpisodeInfos"], R.child_path(path, "EnableEmbeddedEpisodeInfos"), state, depth + 1)
    end
    if value["EnableEmbeddedExtrasTitles"] ~= nil then
        V.check_Schema785(J, value["EnableEmbeddedExtrasTitles"], R.child_path(path, "EnableEmbeddedExtrasTitles"), state, depth + 1)
    end
    if value["EnableEmbeddedTitles"] ~= nil then
        V.check_Schema786(J, value["EnableEmbeddedTitles"], R.child_path(path, "EnableEmbeddedTitles"), state, depth + 1)
    end
    if value["EnableInternetProviders"] ~= nil then
        V.check_Schema787(J, value["EnableInternetProviders"], R.child_path(path, "EnableInternetProviders"), state, depth + 1)
    end
    if value["EnableLUFSScan"] ~= nil then
        V.check_Schema788(J, value["EnableLUFSScan"], R.child_path(path, "EnableLUFSScan"), state, depth + 1)
    end
    if value["EnablePhotos"] ~= nil then
        V.check_Schema789(J, value["EnablePhotos"], R.child_path(path, "EnablePhotos"), state, depth + 1)
    end
    if value["EnableRealtimeMonitor"] ~= nil then
        V.check_Schema790(J, value["EnableRealtimeMonitor"], R.child_path(path, "EnableRealtimeMonitor"), state, depth + 1)
    end
    if value["EnableTrickplayImageExtraction"] ~= nil then
        V.check_Schema791(J, value["EnableTrickplayImageExtraction"], R.child_path(path, "EnableTrickplayImageExtraction"), state, depth + 1)
    end
    if value["Enabled"] ~= nil then
        V.check_Schema792(J, value["Enabled"], R.child_path(path, "Enabled"), state, depth + 1)
    end
    if value["ExtractChapterImagesDuringLibraryScan"] ~= nil then
        V.check_Schema793(J, value["ExtractChapterImagesDuringLibraryScan"], R.child_path(path, "ExtractChapterImagesDuringLibraryScan"), state, depth + 1)
    end
    if value["ExtractTrickplayImagesDuringLibraryScan"] ~= nil then
        V.check_Schema794(J, value["ExtractTrickplayImagesDuringLibraryScan"], R.child_path(path, "ExtractTrickplayImagesDuringLibraryScan"), state, depth + 1)
    end
    if value["LocalMetadataReaderOrder"] ~= nil then
        V.check_Schema795(J, value["LocalMetadataReaderOrder"], R.child_path(path, "LocalMetadataReaderOrder"), state, depth + 1)
    end
    if value["LyricFetcherOrder"] ~= nil then
        V.check_Schema796(J, value["LyricFetcherOrder"], R.child_path(path, "LyricFetcherOrder"), state, depth + 1)
    end
    if value["MediaSegmentProvideOrder"] ~= nil then
        V.check_Schema797(J, value["MediaSegmentProvideOrder"], R.child_path(path, "MediaSegmentProvideOrder"), state, depth + 1)
    end
    if value["MetadataCountryCode"] ~= nil then
        V.check_Schema798(J, value["MetadataCountryCode"], R.child_path(path, "MetadataCountryCode"), state, depth + 1)
    end
    if value["MetadataSavers"] ~= nil then
        V.check_Schema799(J, value["MetadataSavers"], R.child_path(path, "MetadataSavers"), state, depth + 1)
    end
    if value["PathInfos"] ~= nil then
        V.check_Schema800(J, value["PathInfos"], R.child_path(path, "PathInfos"), state, depth + 1)
    end
    if value["PreferNonstandardArtistsTag"] ~= nil then
        V.check_Schema801(J, value["PreferNonstandardArtistsTag"], R.child_path(path, "PreferNonstandardArtistsTag"), state, depth + 1)
    end
    if value["PreferredMetadataLanguage"] ~= nil then
        V.check_Schema802(J, value["PreferredMetadataLanguage"], R.child_path(path, "PreferredMetadataLanguage"), state, depth + 1)
    end
    if value["RequirePerfectSubtitleMatch"] ~= nil then
        V.check_Schema803(J, value["RequirePerfectSubtitleMatch"], R.child_path(path, "RequirePerfectSubtitleMatch"), state, depth + 1)
    end
    if value["SaveLocalMetadata"] ~= nil then
        V.check_Schema804(J, value["SaveLocalMetadata"], R.child_path(path, "SaveLocalMetadata"), state, depth + 1)
    end
    if value["SaveLyricsWithMedia"] ~= nil then
        V.check_Schema805(J, value["SaveLyricsWithMedia"], R.child_path(path, "SaveLyricsWithMedia"), state, depth + 1)
    end
    if value["SaveSubtitlesWithMedia"] ~= nil then
        V.check_Schema806(J, value["SaveSubtitlesWithMedia"], R.child_path(path, "SaveSubtitlesWithMedia"), state, depth + 1)
    end
    if value["SaveTrickplayWithMedia"] ~= nil then
        V.check_Schema807(J, value["SaveTrickplayWithMedia"], R.child_path(path, "SaveTrickplayWithMedia"), state, depth + 1)
    end
    if value["SeasonZeroDisplayName"] ~= nil then
        V.check_Schema808(J, value["SeasonZeroDisplayName"], R.child_path(path, "SeasonZeroDisplayName"), state, depth + 1)
    end
    if value["SkipSubtitlesIfAudioTrackMatches"] ~= nil then
        V.check_Schema809(J, value["SkipSubtitlesIfAudioTrackMatches"], R.child_path(path, "SkipSubtitlesIfAudioTrackMatches"), state, depth + 1)
    end
    if value["SkipSubtitlesIfEmbeddedSubtitlesPresent"] ~= nil then
        V.check_Schema810(J, value["SkipSubtitlesIfEmbeddedSubtitlesPresent"], R.child_path(path, "SkipSubtitlesIfEmbeddedSubtitlesPresent"), state, depth + 1)
    end
    if value["SubtitleDownloadLanguages"] ~= nil then
        V.check_Schema811(J, value["SubtitleDownloadLanguages"], R.child_path(path, "SubtitleDownloadLanguages"), state, depth + 1)
    end
    if value["SubtitleFetcherOrder"] ~= nil then
        V.check_Schema812(J, value["SubtitleFetcherOrder"], R.child_path(path, "SubtitleFetcherOrder"), state, depth + 1)
    end
    if value["TypeOptions"] ~= nil then
        V.check_Schema813(J, value["TypeOptions"], R.child_path(path, "TypeOptions"), state, depth + 1)
    end
    if value["UseCustomTagDelimiters"] ~= nil then
        V.check_Schema814(J, value["UseCustomTagDelimiters"], R.child_path(path, "UseCustomTagDelimiters"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "AllowEmbeddedSubtitles" and key ~= "AutomaticRefreshIntervalDays" and key ~= "AutomaticallyAddToCollection" and key ~= "CustomTagDelimiters" and key ~= "DelimiterWhitelist" and key ~= "DisabledLocalMetadataReaders" and key ~= "DisabledLyricFetchers" and key ~= "DisabledMediaSegmentProviders" and key ~= "DisabledSubtitleFetchers" and key ~= "EnableAutomaticSeriesGrouping" and key ~= "EnableChapterImageExtraction" and key ~= "EnableEmbeddedEpisodeInfos" and key ~= "EnableEmbeddedExtrasTitles" and key ~= "EnableEmbeddedTitles" and key ~= "EnableInternetProviders" and key ~= "EnableLUFSScan" and key ~= "EnablePhotos" and key ~= "EnableRealtimeMonitor" and key ~= "EnableTrickplayImageExtraction" and key ~= "Enabled" and key ~= "ExtractChapterImagesDuringLibraryScan" and key ~= "ExtractTrickplayImagesDuringLibraryScan" and key ~= "LocalMetadataReaderOrder" and key ~= "LyricFetcherOrder" and key ~= "MediaSegmentProvideOrder" and key ~= "MetadataCountryCode" and key ~= "MetadataSavers" and key ~= "PathInfos" and key ~= "PreferNonstandardArtistsTag" and key ~= "PreferredMetadataLanguage" and key ~= "RequirePerfectSubtitleMatch" and key ~= "SaveLocalMetadata" and key ~= "SaveLyricsWithMedia" and key ~= "SaveSubtitlesWithMedia" and key ~= "SaveTrickplayWithMedia" and key ~= "SeasonZeroDisplayName" and key ~= "SkipSubtitlesIfAudioTrackMatches" and key ~= "SkipSubtitlesIfEmbeddedSubtitlesPresent" and key ~= "SubtitleDownloadLanguages" and key ~= "SubtitleFetcherOrder" and key ~= "TypeOptions" and key ~= "UseCustomTagDelimiters" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema773(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"AllowAll", "AllowText", "AllowImage", "AllowNone", }), path)
    V.check_EmbeddedSubtitleOptions(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema774(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema775(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema776(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema815(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema777(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema816(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema778(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema817(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema779(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema818(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema780(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema819(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema781(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema820(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema782(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema783(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema784(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema785(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema786(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema787(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema788(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema789(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema790(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema791(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema792(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema793(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema794(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema795(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema821(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema796(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema822(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema797(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema823(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema798(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema799(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema824(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema800(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_MediaPathInfo(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema801(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema802(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema803(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema804(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema805(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema806(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema807(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema808(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema809(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema810(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema811(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema825(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema812(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema826(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema813(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_TypeOptions(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema814(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema815(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema816(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema817(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema818(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema819(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema820(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema821(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema822(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema823(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema824(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema825(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema826(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_MediaProtocol(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"File", "Http", "Rtmp", "Rtsp", "Udp", "Rtp", "Ftp", }), path)
    return value
end

function V.check_MediaAttachment(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["Codec"] ~= nil then
        V.check_Schema829(J, value["Codec"], R.child_path(path, "Codec"), state, depth + 1)
    end
    if value["CodecTag"] ~= nil then
        V.check_Schema830(J, value["CodecTag"], R.child_path(path, "CodecTag"), state, depth + 1)
    end
    if value["Comment"] ~= nil then
        V.check_Schema831(J, value["Comment"], R.child_path(path, "Comment"), state, depth + 1)
    end
    if value["DeliveryUrl"] ~= nil then
        V.check_Schema832(J, value["DeliveryUrl"], R.child_path(path, "DeliveryUrl"), state, depth + 1)
    end
    if value["FileName"] ~= nil then
        V.check_Schema833(J, value["FileName"], R.child_path(path, "FileName"), state, depth + 1)
    end
    if value["Index"] ~= nil then
        V.check_Schema834(J, value["Index"], R.child_path(path, "Index"), state, depth + 1)
    end
    if value["MimeType"] ~= nil then
        V.check_Schema835(J, value["MimeType"], R.child_path(path, "MimeType"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "Codec" and key ~= "CodecTag" and key ~= "Comment" and key ~= "DeliveryUrl" and key ~= "FileName" and key ~= "Index" and key ~= "MimeType" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema829(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema830(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema831(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema832(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema833(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema834(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema835(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_TransportStreamTimestamp(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"None", "Zero", "Valid", }), path)
    return value
end

function V.check_MediaStreamProtocol(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"http", "hls", }), path)
    return value
end

function V.check_MediaSourceType(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Default", "Grouping", "Placeholder", }), path)
    return value
end

function V.check_AudioSpatialFormat(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"None", "DolbyAtmos", "DTSX", }), path)
    return value
end

function V.check_MediaStreamType(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Audio", "Video", "Subtitle", "EmbeddedImage", "Data", "Lyric", }), path)
    return value
end

function V.check_VideoRange(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Unknown", "SDR", "HDR", }), path)
    return value
end

function V.check_VideoRangeType(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Unknown", "SDR", "HDR10", "HLG", "DOVI", "DOVIWithHDR10", "DOVIWithHLG", "DOVIWithSDR", "HDR10Plus", }), path)
    return value
end

function V.check_PersonKind(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Unknown", "Actor", "Director", "Composer", "Writer", "GuestStar", "Producer", "Conductor", "Lyricist", "Arranger", "Engineer", "Mixer", "Remixer", "Creator", "Artist", "AlbumArtist", "Author", "Illustrator", "Penciller", "Inker", "Colorist", "Letterer", "CoverArtist", "Editor", "Translator", }), path)
    return value
end

function V.check_SessionUserInfo(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["UserId"] ~= nil then
        V.check_Schema845(J, value["UserId"], R.child_path(path, "UserId"), state, depth + 1)
    end
    if value["UserName"] ~= nil then
        V.check_Schema846(J, value["UserName"], R.child_path(path, "UserName"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "UserId" and key ~= "UserName" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema845(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema846(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_ClientCapabilitiesDto(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["AppStoreUrl"] ~= nil then
        V.check_Schema848(J, value["AppStoreUrl"], R.child_path(path, "AppStoreUrl"), state, depth + 1)
    end
    if value["DeviceProfile"] ~= nil then
        V.check_Schema849(J, value["DeviceProfile"], R.child_path(path, "DeviceProfile"), state, depth + 1)
    end
    if value["IconUrl"] ~= nil then
        V.check_Schema850(J, value["IconUrl"], R.child_path(path, "IconUrl"), state, depth + 1)
    end
    if value["PlayableMediaTypes"] ~= nil then
        V.check_Schema851(J, value["PlayableMediaTypes"], R.child_path(path, "PlayableMediaTypes"), state, depth + 1)
    end
    if value["SupportedCommands"] ~= nil then
        V.check_Schema852(J, value["SupportedCommands"], R.child_path(path, "SupportedCommands"), state, depth + 1)
    end
    if value["SupportsMediaControl"] ~= nil then
        V.check_Schema853(J, value["SupportsMediaControl"], R.child_path(path, "SupportsMediaControl"), state, depth + 1)
    end
    if value["SupportsPersistentIdentifier"] ~= nil then
        V.check_Schema854(J, value["SupportsPersistentIdentifier"], R.child_path(path, "SupportsPersistentIdentifier"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "AppStoreUrl" and key ~= "DeviceProfile" and key ~= "IconUrl" and key ~= "PlayableMediaTypes" and key ~= "SupportedCommands" and key ~= "SupportsMediaControl" and key ~= "SupportsPersistentIdentifier" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema848(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema849(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    V.check_DeviceProfile(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema850(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema851(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_MediaType(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema852(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_GeneralCommandType(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema853(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema854(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_QueueItem(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["Id"] ~= nil then
        V.check_Schema856(J, value["Id"], R.child_path(path, "Id"), state, depth + 1)
    end
    if value["PlaylistItemId"] ~= nil then
        V.check_Schema857(J, value["PlaylistItemId"], R.child_path(path, "PlaylistItemId"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "Id" and key ~= "PlaylistItemId" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema856(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema857(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_PlayerStateInfo(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["AudioStreamIndex"] ~= nil then
        V.check_Schema859(J, value["AudioStreamIndex"], R.child_path(path, "AudioStreamIndex"), state, depth + 1)
    end
    if value["CanSeek"] ~= nil then
        V.check_Schema860(J, value["CanSeek"], R.child_path(path, "CanSeek"), state, depth + 1)
    end
    if value["IsMuted"] ~= nil then
        V.check_Schema861(J, value["IsMuted"], R.child_path(path, "IsMuted"), state, depth + 1)
    end
    if value["IsPaused"] ~= nil then
        V.check_Schema862(J, value["IsPaused"], R.child_path(path, "IsPaused"), state, depth + 1)
    end
    if value["LiveStreamId"] ~= nil then
        V.check_Schema863(J, value["LiveStreamId"], R.child_path(path, "LiveStreamId"), state, depth + 1)
    end
    if value["MediaSourceId"] ~= nil then
        V.check_Schema864(J, value["MediaSourceId"], R.child_path(path, "MediaSourceId"), state, depth + 1)
    end
    if value["PlayMethod"] ~= nil then
        V.check_Schema865(J, value["PlayMethod"], R.child_path(path, "PlayMethod"), state, depth + 1)
    end
    if value["PlaybackOrder"] ~= nil then
        V.check_Schema866(J, value["PlaybackOrder"], R.child_path(path, "PlaybackOrder"), state, depth + 1)
    end
    if value["PositionTicks"] ~= nil then
        V.check_Schema867(J, value["PositionTicks"], R.child_path(path, "PositionTicks"), state, depth + 1)
    end
    if value["RepeatMode"] ~= nil then
        V.check_Schema868(J, value["RepeatMode"], R.child_path(path, "RepeatMode"), state, depth + 1)
    end
    if value["SubtitleStreamIndex"] ~= nil then
        V.check_Schema869(J, value["SubtitleStreamIndex"], R.child_path(path, "SubtitleStreamIndex"), state, depth + 1)
    end
    if value["VolumeLevel"] ~= nil then
        V.check_Schema870(J, value["VolumeLevel"], R.child_path(path, "VolumeLevel"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "AudioStreamIndex" and key ~= "CanSeek" and key ~= "IsMuted" and key ~= "IsPaused" and key ~= "LiveStreamId" and key ~= "MediaSourceId" and key ~= "PlayMethod" and key ~= "PlaybackOrder" and key ~= "PositionTicks" and key ~= "RepeatMode" and key ~= "SubtitleStreamIndex" and key ~= "VolumeLevel" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema859(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema860(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema861(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema862(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema863(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema864(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema865(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.enum(J, value, J.array({"Transcode", "DirectStream", "DirectPlay", }), path)
    V.check_PlayMethod(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema866(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"Default", "Shuffle", }), path)
    V.check_PlaybackOrder(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema867(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    return value
end

function V.check_Schema868(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"RepeatNone", "RepeatAll", "RepeatOne", }), path)
    V.check_RepeatMode(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema869(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema870(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_GeneralCommandType(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"MoveUp", "MoveDown", "MoveLeft", "MoveRight", "PageUp", "PageDown", "PreviousLetter", "NextLetter", "ToggleOsd", "ToggleContextMenu", "Select", "Back", "TakeScreenshot", "SendKey", "SendString", "GoHome", "GoToSettings", "VolumeUp", "VolumeDown", "Mute", "Unmute", "ToggleMute", "SetVolume", "SetAudioStreamIndex", "SetSubtitleStreamIndex", "ToggleFullscreen", "DisplayContent", "GoToSearch", "DisplayMessage", "SetRepeatMode", "ChannelUp", "ChannelDown", "Guide", "ToggleStats", "PlayMediaSource", "PlayTrailers", "SetShuffleQueue", "PlayState", "PlayNext", "ToggleOsdMenu", "Play", "SetMaxStreamingBitrate", "SetPlaybackOrder", }), path)
    return value
end

function V.check_TranscodingInfo(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["AudioChannels"] ~= nil then
        V.check_Schema873(J, value["AudioChannels"], R.child_path(path, "AudioChannels"), state, depth + 1)
    end
    if value["AudioCodec"] ~= nil then
        V.check_Schema874(J, value["AudioCodec"], R.child_path(path, "AudioCodec"), state, depth + 1)
    end
    if value["Bitrate"] ~= nil then
        V.check_Schema875(J, value["Bitrate"], R.child_path(path, "Bitrate"), state, depth + 1)
    end
    if value["CompletionPercentage"] ~= nil then
        V.check_Schema876(J, value["CompletionPercentage"], R.child_path(path, "CompletionPercentage"), state, depth + 1)
    end
    if value["Container"] ~= nil then
        V.check_Schema877(J, value["Container"], R.child_path(path, "Container"), state, depth + 1)
    end
    if value["Framerate"] ~= nil then
        V.check_Schema878(J, value["Framerate"], R.child_path(path, "Framerate"), state, depth + 1)
    end
    if value["HardwareAccelerationType"] ~= nil then
        V.check_Schema879(J, value["HardwareAccelerationType"], R.child_path(path, "HardwareAccelerationType"), state, depth + 1)
    end
    if value["Height"] ~= nil then
        V.check_Schema880(J, value["Height"], R.child_path(path, "Height"), state, depth + 1)
    end
    if value["IsAudioDirect"] ~= nil then
        V.check_Schema881(J, value["IsAudioDirect"], R.child_path(path, "IsAudioDirect"), state, depth + 1)
    end
    if value["IsVideoDirect"] ~= nil then
        V.check_Schema882(J, value["IsVideoDirect"], R.child_path(path, "IsVideoDirect"), state, depth + 1)
    end
    if value["TranscodeReasons"] ~= nil then
        V.check_Schema883(J, value["TranscodeReasons"], R.child_path(path, "TranscodeReasons"), state, depth + 1)
    end
    if value["VideoCodec"] ~= nil then
        V.check_Schema884(J, value["VideoCodec"], R.child_path(path, "VideoCodec"), state, depth + 1)
    end
    if value["Width"] ~= nil then
        V.check_Schema885(J, value["Width"], R.child_path(path, "Width"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "AudioChannels" and key ~= "AudioCodec" and key ~= "Bitrate" and key ~= "CompletionPercentage" and key ~= "Container" and key ~= "Framerate" and key ~= "HardwareAccelerationType" and key ~= "Height" and key ~= "IsAudioDirect" and key ~= "IsVideoDirect" and key ~= "TranscodeReasons" and key ~= "VideoCodec" and key ~= "Width" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema873(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema874(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema875(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema876(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema877(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema878(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema879(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.enum(J, value, J.array({"none", "amf", "qsv", "nvenc", "v4l2m2m", "vaapi", "videotoolbox", "rkmpp", }), path)
    V.check_HardwareAccelerationType(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema880(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema881(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema882(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema883(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_TranscodeReason(J, child, path .. '/' .. index, state, depth + 1)
    end
    R.enum(J, value, J.array({"ContainerNotSupported", "VideoCodecNotSupported", "AudioCodecNotSupported", "SubtitleCodecNotSupported", "AudioIsExternal", "SecondaryAudioNotSupported", "VideoProfileNotSupported", "VideoLevelNotSupported", "VideoResolutionNotSupported", "VideoBitDepthNotSupported", "VideoFramerateNotSupported", "RefFramesNotSupported", "AnamorphicVideoNotSupported", "InterlacedVideoNotSupported", "AudioChannelsNotSupported", "AudioProfileNotSupported", "AudioSampleRateNotSupported", "AudioBitDepthNotSupported", "ContainerBitrateExceedsLimit", "VideoBitrateNotSupported", "AudioBitrateNotSupported", "UnknownVideoStreamInfo", "UnknownAudioStreamInfo", "DirectPlayError", "VideoRangeTypeNotSupported", "VideoCodecTagNotSupported", }), path)
    return value
end

function V.check_Schema884(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema885(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_UserConfiguration(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["AudioLanguagePreference"] ~= nil then
        V.check_Schema887(J, value["AudioLanguagePreference"], R.child_path(path, "AudioLanguagePreference"), state, depth + 1)
    end
    if value["CastReceiverId"] ~= nil then
        V.check_Schema888(J, value["CastReceiverId"], R.child_path(path, "CastReceiverId"), state, depth + 1)
    end
    if value["DisplayCollectionsView"] ~= nil then
        V.check_Schema889(J, value["DisplayCollectionsView"], R.child_path(path, "DisplayCollectionsView"), state, depth + 1)
    end
    if value["DisplayMissingEpisodes"] ~= nil then
        V.check_Schema890(J, value["DisplayMissingEpisodes"], R.child_path(path, "DisplayMissingEpisodes"), state, depth + 1)
    end
    if value["EnableLocalPassword"] ~= nil then
        V.check_Schema891(J, value["EnableLocalPassword"], R.child_path(path, "EnableLocalPassword"), state, depth + 1)
    end
    if value["EnableNextEpisodeAutoPlay"] ~= nil then
        V.check_Schema892(J, value["EnableNextEpisodeAutoPlay"], R.child_path(path, "EnableNextEpisodeAutoPlay"), state, depth + 1)
    end
    if value["GroupedFolders"] ~= nil then
        V.check_Schema893(J, value["GroupedFolders"], R.child_path(path, "GroupedFolders"), state, depth + 1)
    end
    if value["HidePlayedInLatest"] ~= nil then
        V.check_Schema894(J, value["HidePlayedInLatest"], R.child_path(path, "HidePlayedInLatest"), state, depth + 1)
    end
    if value["LatestItemsExcludes"] ~= nil then
        V.check_Schema895(J, value["LatestItemsExcludes"], R.child_path(path, "LatestItemsExcludes"), state, depth + 1)
    end
    if value["MyMediaExcludes"] ~= nil then
        V.check_Schema896(J, value["MyMediaExcludes"], R.child_path(path, "MyMediaExcludes"), state, depth + 1)
    end
    if value["OrderedViews"] ~= nil then
        V.check_Schema897(J, value["OrderedViews"], R.child_path(path, "OrderedViews"), state, depth + 1)
    end
    if value["PlayDefaultAudioTrack"] ~= nil then
        V.check_Schema898(J, value["PlayDefaultAudioTrack"], R.child_path(path, "PlayDefaultAudioTrack"), state, depth + 1)
    end
    if value["RememberAudioSelections"] ~= nil then
        V.check_Schema899(J, value["RememberAudioSelections"], R.child_path(path, "RememberAudioSelections"), state, depth + 1)
    end
    if value["RememberSubtitleSelections"] ~= nil then
        V.check_Schema900(J, value["RememberSubtitleSelections"], R.child_path(path, "RememberSubtitleSelections"), state, depth + 1)
    end
    if value["SubtitleLanguagePreference"] ~= nil then
        V.check_Schema901(J, value["SubtitleLanguagePreference"], R.child_path(path, "SubtitleLanguagePreference"), state, depth + 1)
    end
    if value["SubtitleMode"] ~= nil then
        V.check_Schema902(J, value["SubtitleMode"], R.child_path(path, "SubtitleMode"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "AudioLanguagePreference" and key ~= "CastReceiverId" and key ~= "DisplayCollectionsView" and key ~= "DisplayMissingEpisodes" and key ~= "EnableLocalPassword" and key ~= "EnableNextEpisodeAutoPlay" and key ~= "GroupedFolders" and key ~= "HidePlayedInLatest" and key ~= "LatestItemsExcludes" and key ~= "MyMediaExcludes" and key ~= "OrderedViews" and key ~= "PlayDefaultAudioTrack" and key ~= "RememberAudioSelections" and key ~= "RememberSubtitleSelections" and key ~= "SubtitleLanguagePreference" and key ~= "SubtitleMode" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema887(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema888(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema889(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema890(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema891(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema892(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema893(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema903(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema894(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema895(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema904(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema896(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema905(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema897(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema906(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema898(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema899(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema900(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema901(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema902(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"Default", "Always", "OnlyForced", "None", "Smart", }), path)
    V.check_SubtitlePlaybackMode(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema903(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema904(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema905(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema906(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_UserPolicy(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["AccessSchedules"] ~= nil then
        V.check_Schema908(J, value["AccessSchedules"], R.child_path(path, "AccessSchedules"), state, depth + 1)
    end
    if value["AllowedTags"] ~= nil then
        V.check_Schema909(J, value["AllowedTags"], R.child_path(path, "AllowedTags"), state, depth + 1)
    end
    V.check_Schema910(J, value["AuthenticationProviderId"], R.child_path(path, "AuthenticationProviderId"), state, depth + 1)
    if value["BlockUnratedItems"] ~= nil then
        V.check_Schema911(J, value["BlockUnratedItems"], R.child_path(path, "BlockUnratedItems"), state, depth + 1)
    end
    if value["BlockedChannels"] ~= nil then
        V.check_Schema912(J, value["BlockedChannels"], R.child_path(path, "BlockedChannels"), state, depth + 1)
    end
    if value["BlockedMediaFolders"] ~= nil then
        V.check_Schema913(J, value["BlockedMediaFolders"], R.child_path(path, "BlockedMediaFolders"), state, depth + 1)
    end
    if value["BlockedTags"] ~= nil then
        V.check_Schema914(J, value["BlockedTags"], R.child_path(path, "BlockedTags"), state, depth + 1)
    end
    if value["EnableAllChannels"] ~= nil then
        V.check_Schema915(J, value["EnableAllChannels"], R.child_path(path, "EnableAllChannels"), state, depth + 1)
    end
    if value["EnableAllDevices"] ~= nil then
        V.check_Schema916(J, value["EnableAllDevices"], R.child_path(path, "EnableAllDevices"), state, depth + 1)
    end
    if value["EnableAllFolders"] ~= nil then
        V.check_Schema917(J, value["EnableAllFolders"], R.child_path(path, "EnableAllFolders"), state, depth + 1)
    end
    if value["EnableAudioPlaybackTranscoding"] ~= nil then
        V.check_Schema918(J, value["EnableAudioPlaybackTranscoding"], R.child_path(path, "EnableAudioPlaybackTranscoding"), state, depth + 1)
    end
    if value["EnableCollectionManagement"] ~= nil then
        V.check_Schema919(J, value["EnableCollectionManagement"], R.child_path(path, "EnableCollectionManagement"), state, depth + 1)
    end
    if value["EnableContentDeletion"] ~= nil then
        V.check_Schema920(J, value["EnableContentDeletion"], R.child_path(path, "EnableContentDeletion"), state, depth + 1)
    end
    if value["EnableContentDeletionFromFolders"] ~= nil then
        V.check_Schema921(J, value["EnableContentDeletionFromFolders"], R.child_path(path, "EnableContentDeletionFromFolders"), state, depth + 1)
    end
    if value["EnableContentDownloading"] ~= nil then
        V.check_Schema922(J, value["EnableContentDownloading"], R.child_path(path, "EnableContentDownloading"), state, depth + 1)
    end
    if value["EnableLiveTvAccess"] ~= nil then
        V.check_Schema923(J, value["EnableLiveTvAccess"], R.child_path(path, "EnableLiveTvAccess"), state, depth + 1)
    end
    if value["EnableLiveTvManagement"] ~= nil then
        V.check_Schema924(J, value["EnableLiveTvManagement"], R.child_path(path, "EnableLiveTvManagement"), state, depth + 1)
    end
    if value["EnableLyricManagement"] ~= nil then
        V.check_Schema925(J, value["EnableLyricManagement"], R.child_path(path, "EnableLyricManagement"), state, depth + 1)
    end
    if value["EnableMediaConversion"] ~= nil then
        V.check_Schema926(J, value["EnableMediaConversion"], R.child_path(path, "EnableMediaConversion"), state, depth + 1)
    end
    if value["EnableMediaPlayback"] ~= nil then
        V.check_Schema927(J, value["EnableMediaPlayback"], R.child_path(path, "EnableMediaPlayback"), state, depth + 1)
    end
    if value["EnablePlaybackRemuxing"] ~= nil then
        V.check_Schema928(J, value["EnablePlaybackRemuxing"], R.child_path(path, "EnablePlaybackRemuxing"), state, depth + 1)
    end
    if value["EnablePublicSharing"] ~= nil then
        V.check_Schema929(J, value["EnablePublicSharing"], R.child_path(path, "EnablePublicSharing"), state, depth + 1)
    end
    if value["EnableRemoteAccess"] ~= nil then
        V.check_Schema930(J, value["EnableRemoteAccess"], R.child_path(path, "EnableRemoteAccess"), state, depth + 1)
    end
    if value["EnableRemoteControlOfOtherUsers"] ~= nil then
        V.check_Schema931(J, value["EnableRemoteControlOfOtherUsers"], R.child_path(path, "EnableRemoteControlOfOtherUsers"), state, depth + 1)
    end
    if value["EnableSharedDeviceControl"] ~= nil then
        V.check_Schema932(J, value["EnableSharedDeviceControl"], R.child_path(path, "EnableSharedDeviceControl"), state, depth + 1)
    end
    if value["EnableSubtitleManagement"] ~= nil then
        V.check_Schema933(J, value["EnableSubtitleManagement"], R.child_path(path, "EnableSubtitleManagement"), state, depth + 1)
    end
    if value["EnableSyncTranscoding"] ~= nil then
        V.check_Schema934(J, value["EnableSyncTranscoding"], R.child_path(path, "EnableSyncTranscoding"), state, depth + 1)
    end
    if value["EnableUserPreferenceAccess"] ~= nil then
        V.check_Schema935(J, value["EnableUserPreferenceAccess"], R.child_path(path, "EnableUserPreferenceAccess"), state, depth + 1)
    end
    if value["EnableVideoPlaybackTranscoding"] ~= nil then
        V.check_Schema936(J, value["EnableVideoPlaybackTranscoding"], R.child_path(path, "EnableVideoPlaybackTranscoding"), state, depth + 1)
    end
    if value["EnabledChannels"] ~= nil then
        V.check_Schema937(J, value["EnabledChannels"], R.child_path(path, "EnabledChannels"), state, depth + 1)
    end
    if value["EnabledDevices"] ~= nil then
        V.check_Schema938(J, value["EnabledDevices"], R.child_path(path, "EnabledDevices"), state, depth + 1)
    end
    if value["EnabledFolders"] ~= nil then
        V.check_Schema939(J, value["EnabledFolders"], R.child_path(path, "EnabledFolders"), state, depth + 1)
    end
    if value["ForceRemoteSourceTranscoding"] ~= nil then
        V.check_Schema940(J, value["ForceRemoteSourceTranscoding"], R.child_path(path, "ForceRemoteSourceTranscoding"), state, depth + 1)
    end
    if value["InvalidLoginAttemptCount"] ~= nil then
        V.check_Schema941(J, value["InvalidLoginAttemptCount"], R.child_path(path, "InvalidLoginAttemptCount"), state, depth + 1)
    end
    if value["IsAdministrator"] ~= nil then
        V.check_Schema942(J, value["IsAdministrator"], R.child_path(path, "IsAdministrator"), state, depth + 1)
    end
    if value["IsDisabled"] ~= nil then
        V.check_Schema943(J, value["IsDisabled"], R.child_path(path, "IsDisabled"), state, depth + 1)
    end
    if value["IsHidden"] ~= nil then
        V.check_Schema944(J, value["IsHidden"], R.child_path(path, "IsHidden"), state, depth + 1)
    end
    if value["LoginAttemptsBeforeLockout"] ~= nil then
        V.check_Schema945(J, value["LoginAttemptsBeforeLockout"], R.child_path(path, "LoginAttemptsBeforeLockout"), state, depth + 1)
    end
    if value["MaxActiveSessions"] ~= nil then
        V.check_Schema946(J, value["MaxActiveSessions"], R.child_path(path, "MaxActiveSessions"), state, depth + 1)
    end
    if value["MaxParentalRating"] ~= nil then
        V.check_Schema947(J, value["MaxParentalRating"], R.child_path(path, "MaxParentalRating"), state, depth + 1)
    end
    V.check_Schema948(J, value["PasswordResetProviderId"], R.child_path(path, "PasswordResetProviderId"), state, depth + 1)
    if value["RemoteClientBitrateLimit"] ~= nil then
        V.check_Schema949(J, value["RemoteClientBitrateLimit"], R.child_path(path, "RemoteClientBitrateLimit"), state, depth + 1)
    end
    if value["SyncPlayAccess"] ~= nil then
        V.check_Schema950(J, value["SyncPlayAccess"], R.child_path(path, "SyncPlayAccess"), state, depth + 1)
    end
    R.check(value["AuthenticationProviderId"] ~= nil, "missing required property: AuthenticationProviderId", path)
    R.check(value["PasswordResetProviderId"] ~= nil, "missing required property: PasswordResetProviderId", path)
    for key, child in pairs(value) do
        if key ~= "AccessSchedules" and key ~= "AllowedTags" and key ~= "AuthenticationProviderId" and key ~= "BlockUnratedItems" and key ~= "BlockedChannels" and key ~= "BlockedMediaFolders" and key ~= "BlockedTags" and key ~= "EnableAllChannels" and key ~= "EnableAllDevices" and key ~= "EnableAllFolders" and key ~= "EnableAudioPlaybackTranscoding" and key ~= "EnableCollectionManagement" and key ~= "EnableContentDeletion" and key ~= "EnableContentDeletionFromFolders" and key ~= "EnableContentDownloading" and key ~= "EnableLiveTvAccess" and key ~= "EnableLiveTvManagement" and key ~= "EnableLyricManagement" and key ~= "EnableMediaConversion" and key ~= "EnableMediaPlayback" and key ~= "EnablePlaybackRemuxing" and key ~= "EnablePublicSharing" and key ~= "EnableRemoteAccess" and key ~= "EnableRemoteControlOfOtherUsers" and key ~= "EnableSharedDeviceControl" and key ~= "EnableSubtitleManagement" and key ~= "EnableSyncTranscoding" and key ~= "EnableUserPreferenceAccess" and key ~= "EnableVideoPlaybackTranscoding" and key ~= "EnabledChannels" and key ~= "EnabledDevices" and key ~= "EnabledFolders" and key ~= "ForceRemoteSourceTranscoding" and key ~= "InvalidLoginAttemptCount" and key ~= "IsAdministrator" and key ~= "IsDisabled" and key ~= "IsHidden" and key ~= "LoginAttemptsBeforeLockout" and key ~= "MaxActiveSessions" and key ~= "MaxParentalRating" and key ~= "PasswordResetProviderId" and key ~= "RemoteClientBitrateLimit" and key ~= "SyncPlayAccess" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema908(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_AccessSchedule(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema909(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema951(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema910(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema911(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_UnratedItem(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema912(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema952(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema913(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema953(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema914(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema954(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema915(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema916(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema917(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema918(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema919(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema920(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema921(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema955(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema922(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema923(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema924(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema925(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema926(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema927(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema928(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema929(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema930(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema931(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema932(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema933(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema934(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema935(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema936(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema937(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema956(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema938(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema957(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema939(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema958(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema940(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema941(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema942(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema943(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema944(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema945(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema946(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema947(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema948(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema949(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema950(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"CreateAndJoinGroups", "JoinGroups", "None", }), path)
    V.check_SyncPlayUserAccessType(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema951(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema952(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema953(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema954(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema955(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema956(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema957(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema958(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_EmbeddedSubtitleOptions(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"AllowAll", "AllowText", "AllowImage", "AllowNone", }), path)
    return value
end

function V.check_MediaPathInfo(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["Path"] ~= nil then
        V.check_Schema961(J, value["Path"], R.child_path(path, "Path"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "Path" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema961(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_TypeOptions(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["ImageFetcherOrder"] ~= nil then
        V.check_Schema963(J, value["ImageFetcherOrder"], R.child_path(path, "ImageFetcherOrder"), state, depth + 1)
    end
    if value["ImageFetchers"] ~= nil then
        V.check_Schema964(J, value["ImageFetchers"], R.child_path(path, "ImageFetchers"), state, depth + 1)
    end
    if value["ImageOptions"] ~= nil then
        V.check_Schema965(J, value["ImageOptions"], R.child_path(path, "ImageOptions"), state, depth + 1)
    end
    if value["MetadataFetcherOrder"] ~= nil then
        V.check_Schema966(J, value["MetadataFetcherOrder"], R.child_path(path, "MetadataFetcherOrder"), state, depth + 1)
    end
    if value["MetadataFetchers"] ~= nil then
        V.check_Schema967(J, value["MetadataFetchers"], R.child_path(path, "MetadataFetchers"), state, depth + 1)
    end
    if value["Type"] ~= nil then
        V.check_Schema968(J, value["Type"], R.child_path(path, "Type"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "ImageFetcherOrder" and key ~= "ImageFetchers" and key ~= "ImageOptions" and key ~= "MetadataFetcherOrder" and key ~= "MetadataFetchers" and key ~= "Type" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema963(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema969(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema964(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema970(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema965(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ImageOption(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema966(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema971(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema967(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_Schema972(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema968(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema969(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema970(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema971(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema972(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_DeviceProfile(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["CodecProfiles"] ~= nil then
        V.check_Schema974(J, value["CodecProfiles"], R.child_path(path, "CodecProfiles"), state, depth + 1)
    end
    if value["ContainerProfiles"] ~= nil then
        V.check_Schema975(J, value["ContainerProfiles"], R.child_path(path, "ContainerProfiles"), state, depth + 1)
    end
    if value["DirectPlayProfiles"] ~= nil then
        V.check_Schema976(J, value["DirectPlayProfiles"], R.child_path(path, "DirectPlayProfiles"), state, depth + 1)
    end
    if value["Id"] ~= nil then
        V.check_Schema977(J, value["Id"], R.child_path(path, "Id"), state, depth + 1)
    end
    if value["MaxStaticBitrate"] ~= nil then
        V.check_Schema978(J, value["MaxStaticBitrate"], R.child_path(path, "MaxStaticBitrate"), state, depth + 1)
    end
    if value["MaxStaticMusicBitrate"] ~= nil then
        V.check_Schema979(J, value["MaxStaticMusicBitrate"], R.child_path(path, "MaxStaticMusicBitrate"), state, depth + 1)
    end
    if value["MaxStreamingBitrate"] ~= nil then
        V.check_Schema980(J, value["MaxStreamingBitrate"], R.child_path(path, "MaxStreamingBitrate"), state, depth + 1)
    end
    if value["MusicStreamingTranscodingBitrate"] ~= nil then
        V.check_Schema981(J, value["MusicStreamingTranscodingBitrate"], R.child_path(path, "MusicStreamingTranscodingBitrate"), state, depth + 1)
    end
    if value["Name"] ~= nil then
        V.check_Schema982(J, value["Name"], R.child_path(path, "Name"), state, depth + 1)
    end
    if value["SubtitleProfiles"] ~= nil then
        V.check_Schema983(J, value["SubtitleProfiles"], R.child_path(path, "SubtitleProfiles"), state, depth + 1)
    end
    if value["TranscodingProfiles"] ~= nil then
        V.check_Schema984(J, value["TranscodingProfiles"], R.child_path(path, "TranscodingProfiles"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "CodecProfiles" and key ~= "ContainerProfiles" and key ~= "DirectPlayProfiles" and key ~= "Id" and key ~= "MaxStaticBitrate" and key ~= "MaxStaticMusicBitrate" and key ~= "MaxStreamingBitrate" and key ~= "MusicStreamingTranscodingBitrate" and key ~= "Name" and key ~= "SubtitleProfiles" and key ~= "TranscodingProfiles" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema974(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_CodecProfile(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema975(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ContainerProfile(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema976(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_DirectPlayProfile(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema977(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema978(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema979(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema980(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema981(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema982(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema983(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_SubtitleProfile(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema984(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_TranscodingProfile(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_PlayMethod(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Transcode", "DirectStream", "DirectPlay", }), path)
    return value
end

function V.check_PlaybackOrder(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Default", "Shuffle", }), path)
    return value
end

function V.check_RepeatMode(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"RepeatNone", "RepeatAll", "RepeatOne", }), path)
    return value
end

function V.check_HardwareAccelerationType(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"none", "amf", "qsv", "nvenc", "v4l2m2m", "vaapi", "videotoolbox", "rkmpp", }), path)
    return value
end

function V.check_TranscodeReason(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"ContainerNotSupported", "VideoCodecNotSupported", "AudioCodecNotSupported", "SubtitleCodecNotSupported", "AudioIsExternal", "SecondaryAudioNotSupported", "VideoProfileNotSupported", "VideoLevelNotSupported", "VideoResolutionNotSupported", "VideoBitDepthNotSupported", "VideoFramerateNotSupported", "RefFramesNotSupported", "AnamorphicVideoNotSupported", "InterlacedVideoNotSupported", "AudioChannelsNotSupported", "AudioProfileNotSupported", "AudioSampleRateNotSupported", "AudioBitDepthNotSupported", "ContainerBitrateExceedsLimit", "VideoBitrateNotSupported", "AudioBitrateNotSupported", "UnknownVideoStreamInfo", "UnknownAudioStreamInfo", "DirectPlayError", "VideoRangeTypeNotSupported", "VideoCodecTagNotSupported", }), path)
    return value
end

function V.check_SubtitlePlaybackMode(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Default", "Always", "OnlyForced", "None", "Smart", }), path)
    return value
end

function V.check_AccessSchedule(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["DayOfWeek"] ~= nil then
        V.check_Schema992(J, value["DayOfWeek"], R.child_path(path, "DayOfWeek"), state, depth + 1)
    end
    if value["EndHour"] ~= nil then
        V.check_Schema993(J, value["EndHour"], R.child_path(path, "EndHour"), state, depth + 1)
    end
    if value["Id"] ~= nil then
        V.check_Schema994(J, value["Id"], R.child_path(path, "Id"), state, depth + 1)
    end
    if value["StartHour"] ~= nil then
        V.check_Schema995(J, value["StartHour"], R.child_path(path, "StartHour"), state, depth + 1)
    end
    if value["UserId"] ~= nil then
        V.check_Schema996(J, value["UserId"], R.child_path(path, "UserId"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "DayOfWeek" and key ~= "EndHour" and key ~= "Id" and key ~= "StartHour" and key ~= "UserId" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema992(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Everyday", "Weekday", "Weekend", }), path)
    V.check_DynamicDayOfWeek(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema993(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema994(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema995(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'number' and value == value and math.abs(value) ~= math.huge, 'expected finite number', path)
    return value
end

function V.check_Schema996(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_UnratedItem(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Movie", "Trailer", "Series", "Music", "Book", "LiveTvChannel", "LiveTvProgram", "ChannelContent", "Other", }), path)
    return value
end

function V.check_SyncPlayUserAccessType(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"CreateAndJoinGroups", "JoinGroups", "None", }), path)
    return value
end

function V.check_ImageOption(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["Limit"] ~= nil then
        V.check_Schema1000(J, value["Limit"], R.child_path(path, "Limit"), state, depth + 1)
    end
    if value["MinWidth"] ~= nil then
        V.check_Schema1001(J, value["MinWidth"], R.child_path(path, "MinWidth"), state, depth + 1)
    end
    if value["Type"] ~= nil then
        V.check_Schema1002(J, value["Type"], R.child_path(path, "Type"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "Limit" and key ~= "MinWidth" and key ~= "Type" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema1000(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema1001(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema1002(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"Primary", "Art", "Backdrop", "Banner", "Logo", "Thumb", "Disc", "Box", "Screenshot", "Menu", "Chapter", "BoxRear", "Profile", }), path)
    V.check_ImageType(J, value, path, state, depth + 1)
    return value
end

function V.check_CodecProfile(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["ApplyConditions"] ~= nil then
        V.check_Schema1004(J, value["ApplyConditions"], R.child_path(path, "ApplyConditions"), state, depth + 1)
    end
    if value["Codec"] ~= nil then
        V.check_Schema1005(J, value["Codec"], R.child_path(path, "Codec"), state, depth + 1)
    end
    if value["Conditions"] ~= nil then
        V.check_Schema1006(J, value["Conditions"], R.child_path(path, "Conditions"), state, depth + 1)
    end
    if value["Container"] ~= nil then
        V.check_Schema1007(J, value["Container"], R.child_path(path, "Container"), state, depth + 1)
    end
    if value["SubContainer"] ~= nil then
        V.check_Schema1008(J, value["SubContainer"], R.child_path(path, "SubContainer"), state, depth + 1)
    end
    if value["Type"] ~= nil then
        V.check_Schema1009(J, value["Type"], R.child_path(path, "Type"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "ApplyConditions" and key ~= "Codec" and key ~= "Conditions" and key ~= "Container" and key ~= "SubContainer" and key ~= "Type" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema1004(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ProfileCondition(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema1005(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema1006(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ProfileCondition(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema1007(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema1008(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema1009(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"Video", "VideoAudio", "Audio", }), path)
    V.check_CodecType(J, value, path, state, depth + 1)
    return value
end

function V.check_ContainerProfile(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["Conditions"] ~= nil then
        V.check_Schema1011(J, value["Conditions"], R.child_path(path, "Conditions"), state, depth + 1)
    end
    if value["Container"] ~= nil then
        V.check_Schema1012(J, value["Container"], R.child_path(path, "Container"), state, depth + 1)
    end
    if value["SubContainer"] ~= nil then
        V.check_Schema1013(J, value["SubContainer"], R.child_path(path, "SubContainer"), state, depth + 1)
    end
    if value["Type"] ~= nil then
        V.check_Schema1014(J, value["Type"], R.child_path(path, "Type"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "Conditions" and key ~= "Container" and key ~= "SubContainer" and key ~= "Type" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema1011(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ProfileCondition(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema1012(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema1013(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema1014(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"Audio", "Video", "Photo", "Subtitle", "Lyric", }), path)
    V.check_DlnaProfileType(J, value, path, state, depth + 1)
    return value
end

function V.check_DirectPlayProfile(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["AudioCodec"] ~= nil then
        V.check_Schema1016(J, value["AudioCodec"], R.child_path(path, "AudioCodec"), state, depth + 1)
    end
    if value["Container"] ~= nil then
        V.check_Schema1017(J, value["Container"], R.child_path(path, "Container"), state, depth + 1)
    end
    if value["Type"] ~= nil then
        V.check_Schema1018(J, value["Type"], R.child_path(path, "Type"), state, depth + 1)
    end
    if value["VideoCodec"] ~= nil then
        V.check_Schema1019(J, value["VideoCodec"], R.child_path(path, "VideoCodec"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "AudioCodec" and key ~= "Container" and key ~= "Type" and key ~= "VideoCodec" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema1016(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema1017(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema1018(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"Audio", "Video", "Photo", "Subtitle", "Lyric", }), path)
    V.check_DlnaProfileType(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema1019(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_SubtitleProfile(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["Container"] ~= nil then
        V.check_Schema1021(J, value["Container"], R.child_path(path, "Container"), state, depth + 1)
    end
    if value["DidlMode"] ~= nil then
        V.check_Schema1022(J, value["DidlMode"], R.child_path(path, "DidlMode"), state, depth + 1)
    end
    if value["Format"] ~= nil then
        V.check_Schema1023(J, value["Format"], R.child_path(path, "Format"), state, depth + 1)
    end
    if value["Language"] ~= nil then
        V.check_Schema1024(J, value["Language"], R.child_path(path, "Language"), state, depth + 1)
    end
    if value["Method"] ~= nil then
        V.check_Schema1025(J, value["Method"], R.child_path(path, "Method"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "Container" and key ~= "DidlMode" and key ~= "Format" and key ~= "Language" and key ~= "Method" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema1021(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema1022(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema1023(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema1024(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema1025(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"Encode", "Embed", "External", "Hls", "Drop", }), path)
    V.check_SubtitleDeliveryMethod(J, value, path, state, depth + 1)
    return value
end

function V.check_TranscodingProfile(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["AudioCodec"] ~= nil then
        V.check_Schema1027(J, value["AudioCodec"], R.child_path(path, "AudioCodec"), state, depth + 1)
    end
    if value["BreakOnNonKeyFrames"] ~= nil then
        V.check_Schema1028(J, value["BreakOnNonKeyFrames"], R.child_path(path, "BreakOnNonKeyFrames"), state, depth + 1)
    end
    if value["Conditions"] ~= nil then
        V.check_Schema1029(J, value["Conditions"], R.child_path(path, "Conditions"), state, depth + 1)
    end
    if value["Container"] ~= nil then
        V.check_Schema1030(J, value["Container"], R.child_path(path, "Container"), state, depth + 1)
    end
    if value["Context"] ~= nil then
        V.check_Schema1031(J, value["Context"], R.child_path(path, "Context"), state, depth + 1)
    end
    if value["CopyTimestamps"] ~= nil then
        V.check_Schema1032(J, value["CopyTimestamps"], R.child_path(path, "CopyTimestamps"), state, depth + 1)
    end
    if value["EnableAudioVbrEncoding"] ~= nil then
        V.check_Schema1033(J, value["EnableAudioVbrEncoding"], R.child_path(path, "EnableAudioVbrEncoding"), state, depth + 1)
    end
    if value["EnableMpegtsM2TsMode"] ~= nil then
        V.check_Schema1034(J, value["EnableMpegtsM2TsMode"], R.child_path(path, "EnableMpegtsM2TsMode"), state, depth + 1)
    end
    if value["EnableSubtitlesInManifest"] ~= nil then
        V.check_Schema1035(J, value["EnableSubtitlesInManifest"], R.child_path(path, "EnableSubtitlesInManifest"), state, depth + 1)
    end
    if value["EstimateContentLength"] ~= nil then
        V.check_Schema1036(J, value["EstimateContentLength"], R.child_path(path, "EstimateContentLength"), state, depth + 1)
    end
    if value["MaxAudioChannels"] ~= nil then
        V.check_Schema1037(J, value["MaxAudioChannels"], R.child_path(path, "MaxAudioChannels"), state, depth + 1)
    end
    if value["MinSegments"] ~= nil then
        V.check_Schema1038(J, value["MinSegments"], R.child_path(path, "MinSegments"), state, depth + 1)
    end
    if value["Protocol"] ~= nil then
        V.check_Schema1039(J, value["Protocol"], R.child_path(path, "Protocol"), state, depth + 1)
    end
    if value["SegmentLength"] ~= nil then
        V.check_Schema1040(J, value["SegmentLength"], R.child_path(path, "SegmentLength"), state, depth + 1)
    end
    if value["TranscodeSeekInfo"] ~= nil then
        V.check_Schema1041(J, value["TranscodeSeekInfo"], R.child_path(path, "TranscodeSeekInfo"), state, depth + 1)
    end
    if value["Type"] ~= nil then
        V.check_Schema1042(J, value["Type"], R.child_path(path, "Type"), state, depth + 1)
    end
    if value["VideoCodec"] ~= nil then
        V.check_Schema1043(J, value["VideoCodec"], R.child_path(path, "VideoCodec"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "AudioCodec" and key ~= "BreakOnNonKeyFrames" and key ~= "Conditions" and key ~= "Container" and key ~= "Context" and key ~= "CopyTimestamps" and key ~= "EnableAudioVbrEncoding" and key ~= "EnableMpegtsM2TsMode" and key ~= "EnableSubtitlesInManifest" and key ~= "EstimateContentLength" and key ~= "MaxAudioChannels" and key ~= "MinSegments" and key ~= "Protocol" and key ~= "SegmentLength" and key ~= "TranscodeSeekInfo" and key ~= "Type" and key ~= "VideoCodec" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema1027(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema1028(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema1029(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.array(J, value, path)
    for index, child in ipairs(value) do
        V.check_ProfileCondition(J, child, path .. '/' .. index, state, depth + 1)
    end
    return value
end

function V.check_Schema1030(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema1031(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"Streaming", "Static", }), path)
    V.check_EncodingContext(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema1032(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema1033(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema1034(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema1035(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema1036(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema1037(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_Schema1038(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema1039(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"http", "hls", }), path)
    V.check_MediaStreamProtocol(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema1040(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(math.type(value) == 'integer', 'expected integer', path)
    R.check(value >= -2147483648 and value <= 2147483647, 'int32 out of range', path)
    return value
end

function V.check_Schema1041(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"Auto", "Bytes", }), path)
    V.check_TranscodeSeekInfo(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema1042(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"Audio", "Video", "Photo", "Subtitle", "Lyric", }), path)
    V.check_DlnaProfileType(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema1043(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_DynamicDayOfWeek(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Everyday", "Weekday", "Weekend", }), path)
    return value
end

function V.check_ProfileCondition(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    local length = R.object(J, value, path)
    if value["Condition"] ~= nil then
        V.check_Schema1046(J, value["Condition"], R.child_path(path, "Condition"), state, depth + 1)
    end
    if value["IsRequired"] ~= nil then
        V.check_Schema1047(J, value["IsRequired"], R.child_path(path, "IsRequired"), state, depth + 1)
    end
    if value["Property"] ~= nil then
        V.check_Schema1048(J, value["Property"], R.child_path(path, "Property"), state, depth + 1)
    end
    if value["Value"] ~= nil then
        V.check_Schema1049(J, value["Value"], R.child_path(path, "Value"), state, depth + 1)
    end
    for key, child in pairs(value) do
        if key ~= "Condition" and key ~= "IsRequired" and key ~= "Property" and key ~= "Value" then
            R.check(false, 'unknown property: ' .. key, path)
        end
    end
    return value
end

function V.check_Schema1046(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"Equals", "NotEquals", "LessThanEqual", "GreaterThanEqual", "EqualsAny", }), path)
    V.check_ProfileConditionType(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema1047(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'boolean', 'expected boolean', path)
    return value
end

function V.check_Schema1048(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.enum(J, value, J.array({"AudioChannels", "AudioBitrate", "AudioProfile", "Width", "Height", "Has64BitOffsets", "PacketLength", "VideoBitDepth", "VideoBitrate", "VideoFramerate", "VideoLevel", "VideoProfile", "VideoTimestamp", "IsAnamorphic", "RefFrames", "NumAudioStreams", "NumVideoStreams", "IsSecondaryAudio", "VideoCodecTag", "IsAvc", "IsInterlaced", "AudioSampleRate", "AudioBitDepth", "VideoRangeType", }), path)
    V.check_ProfileConditionValue(J, value, path, state, depth + 1)
    return value
end

function V.check_Schema1049(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    if value == J.null then return value end
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    return value
end

function V.check_CodecType(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Video", "VideoAudio", "Audio", }), path)
    return value
end

function V.check_DlnaProfileType(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Audio", "Video", "Photo", "Subtitle", "Lyric", }), path)
    return value
end

function V.check_TranscodeSeekInfo(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Auto", "Bytes", }), path)
    return value
end

function V.check_ProfileConditionType(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"Equals", "NotEquals", "LessThanEqual", "GreaterThanEqual", "EqualsAny", }), path)
    return value
end

function V.check_ProfileConditionValue(J, value, path, state, depth)
    R.visit(value, path, state, depth)
    R.check(value ~= J.null, 'null is not allowed', path)
    R.check(type(value) == 'string', 'expected string', path)
    local length = utf8.len(value)
    R.check(length ~= nil, 'invalid UTF-8', path)
    R.enum(J, value, J.array({"AudioChannels", "AudioBitrate", "AudioProfile", "Width", "Height", "Has64BitOffsets", "PacketLength", "VideoBitDepth", "VideoBitrate", "VideoFramerate", "VideoLevel", "VideoProfile", "VideoTimestamp", "IsAnamorphic", "RefFrames", "NumAudioStreams", "NumVideoStreams", "IsSecondaryAudio", "VideoCodecTag", "IsAvc", "IsInterlaced", "AudioSampleRate", "AudioBitDepth", "VideoRangeType", }), path)
    return value
end

return V
