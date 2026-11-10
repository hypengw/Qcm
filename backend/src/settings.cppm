module;
#include <rstd/macro.hpp>
export module qcm.backend:settings;
export import :error_cause;
export import rstd.serde;
import liuuid;
import rstd.json;
using rstd::path::Path;
using rstd::path::PathBuf;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend::settings
{
struct Error {
    String     operation;
    PathBuf    path;
    ErrorCause cause;
    template<class E>
    static auto from(ref<str> operation, ref<Path> path, E error) -> Error {
        return { String::make(operation), PathBuf::from(path), error_cause(rstd::move(error)) };
    }
};
struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", ignore_unknown)]]
Setting {
    String      device_id;
    static auto load(ref<Path> data_dir) -> Result<Setting, Error>;
    auto        save(ref<Path> data_dir) const -> Result<empty, Error>;
};

auto Setting::load(ref<Path> data_dir) -> Result<Setting, Error> {
    auto path = PathBuf::from(data_dir).join("setting.json"_str);
    auto text = rstd::fs::read_to_string(path.as_path());
    if (text.is_ok()) {
        return rstd::json::decode<Setting>(text->as_str()).map_err([&](auto error) {
            return Error::from("decode"_str, path.as_path(), rstd::move(error));
        });
    }
    auto error = rstd::move(text).unwrap_err();
    if (error.kind().code != rstd::io::ErrorKind::NotFound)
        return Err(Error::from("read"_str, path.as_path(), rstd::move(error)));
    auto    id = rstd_try(liuuid::Uuid::new_v4(),
                          Error::from("generate device_id"_str, path.as_path(), rstd::move(_e)));
    Setting setting { id.hyphenated() };
    rstd_try(setting.save(data_dir));
    return Ok(rstd::move(setting));
}
auto Setting::save(ref<Path> data_dir) const -> Result<empty, Error> {
    auto path = PathBuf::from(data_dir).join("setting.json"_str);
    rstd_try(rstd::fs::create_dir_all(data_dir),
             Error::from("create directory"_str, data_dir, rstd::move(_e)));
    auto text = rstd_try(rstd::json::encode(*this),
                         Error::from("encode"_str, path.as_path(), rstd::move(_e)));
    rstd_try(rstd::fs::write(path.as_path(), text.as_str().as_bytes()),
             Error::from("write"_str, path.as_path(), rstd::move(_e)));
    return Ok(empty {});
}
} // namespace qcm::backend::settings

export namespace rstd
{
template<>
struct Impl<fmt::Display, qcm::backend::settings::Error> : ImplBase<qcm::backend::settings::Error> {
    auto fmt(fmt::Formatter& f) const -> bool {
        const auto& error = this->self();
        return f.write_fmt(fmt::Arguments::make(
            "cannot {} settings '{}': {}", error.operation, error.path.as_path(), error.cause));
    }
};
template<>
struct Impl<fmt::Debug, qcm::backend::settings::Error>
    : Impl<fmt::Display, qcm::backend::settings::Error> {};
template<>
struct Impl<error::Error, qcm::backend::settings::Error>
    : DefaultInImpl<error::Error, qcm::backend::settings::Error> {
    auto source() const noexcept -> Option<error::ErrorRef> {
        return Some(dyn<error::Error>::from_ref(this->self().cause));
    }
};
} // namespace rstd
