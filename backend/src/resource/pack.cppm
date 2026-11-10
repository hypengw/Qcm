module;
#include <rstd/macro.hpp>
#ifndef _WIN32
#    include <fcntl.h>
#endif
export module qcm.backend:resource.pack;
export import :resource.format;
export import sqlito;
import liuuid;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend::resource_cache
{
struct PackEntry {
    u64           offset, length;
    array<u8, 32> digest;
};
class Pack {
    rstd::fs::File file_;
    explicit Pack(rstd::fs::File file): file_(rstd::move(file)) {}
    static auto error(ref<str> message) -> Error { return Error::state(message); }
    static auto valid_token(ref<str> token) -> bool {
        return token.len() == usize(32) && liuuid::Uuid::parse_str(token).is_ok();
    }
    static auto header(ref<str> token) -> Vec<u8> {
        auto bytes = Vec<u8>::from("QCMPACK1"_str.as_bytes());
        bytes.extend_from_slice(token.as_bytes());
        bytes.resize(usize(64), u8());
        return bytes;
    }
    static auto open_file(ref<rstd::path::Path> root, ref<str> token, bool create)
        -> Result<rstd::fs::File, Error> {
        if (! valid_token(token)) return Err(error("invalid pack token"_str));
        auto path = rstd::path::PathBuf::from(root).join(rstd::format("{}.pack", token).as_str());
        auto info = rstd::fs::symlink_metadata(path.as_path());
        if (info.is_ok() && (! info->is_file() || info->is_symlink()))
            return Err(error("invalid pack file type"_str));
        if (info.is_err()) {
            auto cause = rstd::move(info).unwrap_err();
            if (cause.kind().code != rstd::io::ErrorKind::NotFound || ! create)
                return Err(Error::io(rstd::move(cause)));
        }
        auto options = rstd::fs::File::options();
        options.read(true).write(true).create_new(create).mode(u32(0600));
#ifndef _WIN32
        options.custom_flags(i32(O_NOFOLLOW));
#endif
        auto file = rstd_try(options.open(path.as_path()));
        auto meta = rstd_try(file.metadata());
        if (! meta.is_file()) return Err(error("invalid cache pack"_str));
        return Ok(rstd::move(file));
    }

public:
    Pack(Pack&&) = default;
    static auto create(ref<rstd::path::Path> root, ref<str> token) -> Result<Pack, Error> {
        auto file  = rstd_try(open_file(root, token, true));
        auto bytes = header(token);
        rstd_try(file.write_all_at(bytes.as_slice(), u64()));
        rstd_try(file.sync_all());
        return Ok(Pack(rstd::move(file)));
    }
    static auto open(ref<rstd::path::Path> root, ref<str> token, u64 committed)
        -> Result<Pack, Error> {
        if (committed < u64(64) || committed > u64(16 * 1024 * 1024))
            return Err(error("invalid committed pack length"_str));
        auto file = rstd_try(open_file(root, token, false));
        auto info = rstd_try(file.metadata());
        if (info.len() < committed) return Err(error("incomplete cache pack"_str));
        auto actual = header(token), expected = header(token);
        rstd_try(file.read_exact_at(actual.as_mut_slice().as_mut_ref(), u64()));
        if (actual != expected) return Err(error("pack identity mismatch"_str));
        return Ok(Pack(rstd::move(file)));
    }
    auto recover(u64 committed) -> Result<empty, Error> {
        auto meta = rstd_try(file_.metadata());
        if (committed < u64(64) || committed > u64(16 * 1024 * 1024) || meta.len() < committed)
            return Err(error("invalid pack recovery length"_str));
        if (meta.len() != committed) {
            rstd_try(file_.set_len(committed));
            rstd_try(file_.sync_all());
        }
        return Ok(empty {});
    }
    auto append(u64 committed, slice<u8> bytes) -> Result<PackEntry, Error> {
        auto length = u64(bytes.len().to_primitive());
        if (committed < u64(64) || committed > u64(16 * 1024 * 1024) ||
            length > u64(16 * 1024 * 1024) - committed)
            return Err(error("invalid pack append"_str));
        auto digest = rstd_try(hash(bytes));
        rstd_try(recover(committed));
        auto writing = file_.write_all_at(bytes, committed);
        if (writing.is_ok()) writing = file_.sync_data();
        if (writing.is_err()) {
            (void)file_.set_len(committed);
            (void)file_.sync_data();
            return Err(Error::io(rstd::move(writing).unwrap_err()));
        }
        return Ok(PackEntry { committed, length, digest });
    }
    auto read(u64 committed, const PackEntry& entry) -> Result<Vec<u8>, Error> {
        if (committed > u64(16 * 1024 * 1024) || entry.offset < u64(64) ||
            entry.offset > committed || entry.length > committed - entry.offset)
            return Err(error("invalid packed object"_str));
        auto bytes = Vec<u8>::make();
        bytes.resize(usize(entry.length.to_primitive()), u8());
        rstd_try(file_.read_exact_at(bytes.as_mut_slice().as_mut_ref(), entry.offset));
        if (rstd_try(hash(bytes.as_slice())) != entry.digest)
            return Err(error("corrupt packed object"_str));
        return Ok(rstd::move(bytes));
    }
};
} // namespace qcm::backend::resource_cache
