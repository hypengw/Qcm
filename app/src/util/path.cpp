module;
#include "core/log.h"
#include "core/qstr_helper.h"

#include <cstdlib>

#include <rstd/macro.hpp>
module qcm;
import :util.path;
import qcm.log;

namespace cppstd = rstd::cppstd;

using path = cppstd::filesystem::path;

path qcm::config_path() {
    auto locs = QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation);
    debug_assert(locs.size() > 0);
    return path(locs[0].toStdString());
}

path qcm::data_path() {
    auto locs = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    debug_assert(locs.size() > 0);
    return path(locs[0].toStdString());
}

path qcm::cache_path() {
    auto locs = QStandardPaths::standardLocations(QStandardPaths::CacheLocation);
    debug_assert(locs.size() > 0);
    return path(locs[0].toStdString());
}

bool qcm::init_path(std::span<const path> pathes) {
    for (auto& p : pathes) {
        std::error_code ec;
        cppstd::filesystem::create_directories(p, ec);
        debug_assert(! ec, "path: {}, info: {}({})", p.string(), ec.message(), ec.value());
    }
    return true;
}
