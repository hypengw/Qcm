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
