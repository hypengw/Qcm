#pragma once

#include <string>
#include <variant>
#include <vector>
#include <optional>

#include "core/core.h"

namespace request
{

struct HttpHeader {
    struct Request {
        std::string method;
        std::string version;
        std::string target;
    };
    struct Status {
        std::string version;
        i32         code;
    };
    struct Field {
        std::string name;
        std::string value;
    };
    using Start = std::variant<Request, Status>;

    std::optional<Start> start;
    std::vector<Field>   fields;

    auto has_field(std::string_view) const -> bool;

    static auto parse_header(std::string_view) -> std::optional<HttpHeader>;
    static auto parse_start_line(std::string_view) -> std::optional<Start>;
    static auto parse_field_line(std::string_view) -> Field;
};

} // namespace request