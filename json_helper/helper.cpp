#include "json_helper/helper.h"
#include "json_helper/helper.inl"

using namespace qcm;

using Error = json::Error;
using ET    = Error::Id;
using njson = nlohmann::json;

void json::detail::deleter(njson* j) {
    if (j != nullptr) delete j;
}

nstd::expected<json::up_njson, Error> json::parse(std::string_view source) {
    try {
        return up_njson(new njson(njson::parse(source)), &detail::deleter);
    } catch (njson::parse_error& e) {
        return nstd::unexpected(Error { .id = ET::ParseError, .what = e.what() });
    }
}

JSON_GET_IMPL(njson);

JSON_GET_IMPL(bool);
JSON_GET_IMPL(int);
JSON_GET_IMPL(i64);
JSON_GET_IMPL(std::string);
JSON_GET_IMPL(std::vector<std::string>);

IMPL_CONVERT(std::string, qcm::json::njson) { out = in.dump(); }