export module qcm.backend:resource.pack;
export import :resource.error;
using namespace rstd::prelude;

export namespace qcm::backend::resource_cache
{
struct PackEntry {
    u64           offset, length;
    array<u8, 32> digest;
};
class Pack {
    rstd::fs::File file_;
    explicit Pack(rstd::fs::File file);
    static auto error(ref<str> message) -> Error;
    static auto valid_token(ref<str> token) -> bool;
    static auto header(ref<str> token) -> Vec<u8>;
    static auto open_file(ref<rstd::path::Path> root, ref<str> token, bool create)
        -> Result<rstd::fs::File, Error>;

public:
    Pack(Pack&&) = default;
    static auto create(ref<rstd::path::Path> root, ref<str> token) -> Result<Pack, Error>;
    static auto open(ref<rstd::path::Path> root, ref<str> token, u64 committed)
        -> Result<Pack, Error>;
    auto recover(u64 committed) -> Result<empty, Error>;
    auto append(u64 committed, slice<u8> bytes) -> Result<PackEntry, Error>;
    auto read(u64 committed, const PackEntry& entry) -> Result<Vec<u8>, Error>;
};
} // namespace qcm::backend::resource_cache
