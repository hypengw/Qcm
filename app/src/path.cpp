#include "Qcm/path.h"
#include "qcm_interface/type.h"
#include "core/log.h"

#include <cstdlib>
#include <QStandardPaths>

using path = std::filesystem::path;

std::filesystem::path qcm::config_path() {
    auto locs = QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation);
    _assert_(locs.size() > 0);
    return std::filesystem::path(convert_from<std::string>(locs[0]));
}

std::filesystem::path qcm::data_path() {
    auto locs = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    _assert_(locs.size() > 0);
    return std::filesystem::path(convert_from<std::string>(locs[0]));
}

std::filesystem::path qcm::cache_path() {
    auto locs = QStandardPaths::standardLocations(QStandardPaths::CacheLocation);
    _assert_(locs.size() > 0);
    return std::filesystem::path(convert_from<std::string>(locs[0]));
}

bool qcm::init_path(std::span<const std::filesystem::path> pathes) {
    for (auto& p : pathes) {
        std::error_code ec;
        std::filesystem::create_directories(p, ec);
        _assert_msg_(! ec, "path: {}, info: {}({})", p.native(), ec.message(), ec.value());
    }
    return true;
}
