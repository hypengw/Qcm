#include "dump.h"
#include <nlohmann/json.hpp>

#include "ncm/client.h"

using namespace ncm;

template<typename T>
std::string ncm::to_json_str(const T& v) {
    return json::njson(v).dump();
}

#define IMPL(TYPE) template std::string ncm::to_json_str<TYPE>(const TYPE&)

IMPL(Params);

auto api::concat_query(std::string_view url, std::string_view query) -> std::string {
    return fmt::format("{}{}{}", url, query.empty() ? "" : "?", query);
}

auto api::format_api(std::string_view path, const UrlParams& query,
                     const Params& body) -> std::string {
    json::njson j, p;
    j["path"] = api::concat_query(path, query.encode());
    for (auto& b : body) {
        p[b.first] = b.second;
    }
    j["param"] = p;
    return j.dump(2);
}

auto api::device_id_from_uuid(std::string_view uuid) -> std::string {
    auto out = std::string(uuid);
    std::erase_if(out, [](auto c) {
        return c == '{' || c == '}' || c == '-';
    });
    return out;
}