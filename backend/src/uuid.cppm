export module qcm.backend:uuid;
export import liuuid;
using namespace rstd::prelude;

export namespace qcm::backend
{
auto random_uuid() -> Result<String, rstd::io::Error> {
    return liuuid::Uuid::new_v4().map([](auto value) {
        return value.hyphenated();
    });
}
} // namespace qcm::backend
