#include "json_helper/helper.h"
#include "json_helper/helper.inl"
#include <fstream>

using Error = qcm::json::Error;
using ET    = Error::Id;
using njson = nlohmann::json;

namespace qcm
{

void json::detail::deleter(njson* j) {
    if (j != nullptr) delete j;
}

auto json::create() -> up_njson { return up_njson(new njson(), &detail::deleter); }

auto json::parse(const std::filesystem::path& p) -> Result<up_njson> {
    try {
        std::ifstream file(p);
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        return up_njson(new njson(njson::parse(content)), &detail::deleter);
    } catch (njson::parse_error& e) {
        return nstd::unexpected(Error { .id = ET::ParseError, .what = e.what() });
    } catch (const std::ios_base::failure& e) {
        return nstd::unexpected(Error { .id = ET::IoError, .what = e.what() });
    }
}
auto json::parse(std::string_view source) -> Result<up_njson> {
    try {
        return up_njson(new njson(njson::parse(source)), &detail::deleter);
    } catch (njson::parse_error& e) {
        return nstd::unexpected(Error { .id = ET::ParseError, .what = e.what() });
    }
}

auto json::at_keys(const njson& j_in, std::span<const Key> keys) -> const json::njson& {
    return *std::accumulate(keys.begin(), keys.end(), &j_in, [](const njson* j, const Key& key) {
        const njson* out;
        std::visit(overloaded { [&out, j](std::string_view k) {
                                   out = &((*j).at(k));
                               },
                                [&out, j](usize idx) {
                                    out = &((*j).at(idx));
                                } },
                   key);
        return out;
    });
}

auto json::assign_keys(njson& j, std::span<const Key> keys) -> njson& {
    njson* out = &j;
    for (auto& key : keys) {
        std::visit(overloaded { [&out](std::string_view k) {
                                   if (! out->is_object()) {
                                       *out = njson::object();
                                   }
                                   if (! out->contains(k)) {
                                       (*out)[k] = njson();
                                   }
                                   out = &(out->at(k));
                               },
                                [&out](usize k) {
                                    if (! out->is_array()) {
                                        *out = json::njson::array();
                                    }
                                    if (k >= out->size()) {
                                        for (auto i = out->size(); i <= k; i++) {
                                            out->push_back(njson());
                                        }
                                    }
                                    out = &(out->at(k));
                                } },
                   key);
    }
    return *out;
}

auto json::catch_error(const std::function<void()>& func) -> std::optional<Error> {
    using ET = Error::Id;
    try {
        func();
    } catch (const njson::type_error& e) {
        return Error { .id = ET::TypeError, .what = e.what() };
    } catch (const njson::parse_error& e) {
        return Error { .id = ET::ParseError, .what = e.what() };
    } catch (const njson::out_of_range& e) {
        return Error { .id = ET::OutOfRange, .what = e.what() };
    }
    return std::nullopt;
}
} // namespace qcm

JSON_GET_IMPL(njson);
JSON_GET_IMPL(bool);
JSON_GET_IMPL(int);
JSON_GET_IMPL(i64);
JSON_GET_IMPL(std::string);
JSON_GET_IMPL(std::vector<std::string>);

IMPL_CONVERT(std::string, qcm::json::njson) { out = in.dump(); }