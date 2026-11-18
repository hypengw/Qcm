export module qcm.backend:resource.format;
export import rstd;
export import rstd.serde;
export import sqlito;
export import :resource.error;
export import :resource.model;

using namespace rstd::prelude;
using rstd::tuple;
using rstd::bytes::Bytes;

export namespace qcm::backend::resource_cache
{
inline constexpr u64   MIN_BLOCK { 256 * 1024 };
inline constexpr u64   MAX_BLOCK { 1024 * 1024 };
inline constexpr u64   HEADER_LEN { 64 };
inline constexpr usize ENTRY_LEN { 36 };
inline constexpr u64   SEGMENT_BYTES { 16 * 1024 * 1024 };
inline constexpr u64   WINDOW_BYTES { 4 * 1024 * 1024 };

struct Range {
    u64 start {}, end {};
};

auto hash(slice<u8> data) -> Result<array<u8, 32>, Error>;

auto identity(slice<u8> data) -> Result<String, Error>;

struct [[pmacro::derive("serde::derive"), pmacro::helper("serde::derive", ignore_unknown)]]
Geometry {
    u64 length {};
    u64 block { MIN_BLOCK };

    static auto make(u64 length) -> Result<Geometry, Error>;

    auto count() const -> u64 {
        return length / block + (length % block != u64() ? u64(1) : u64());
    }

    auto range(u64 index) const -> Result<Range, Error>;

    auto offset(u32 slot) const -> Result<u64, Error>;

    auto header(ref<str> generation) const -> Result<array<u8, 64>, Error>;
};

[[pmacro::invoke(
    "rstd::Enum", [[derive(Debug, Clone, "serde::derive")]] enum Layout {
                      [[default]] Blocks,
                      [[helper("serde::derive", ignore_unknown)]] Segments { bytes: u64 },
                  } members {
    public:
        static auto for_resource(ResourceKind kind, u64 length) -> Layout;
        auto        segment_bytes() const -> Option<u64>;
        auto        validate(Geometry geometry) const -> Result<empty, Error>;
    })]];

class Index {
    Vec<u8>    bytes_;
    Vec<Range> dirty_;
    Geometry   geometry_;

    static auto load_layout(Geometry geometry, Vec<u8> bytes, Option<u64> slots)
        -> Result<Index, Error>;

public:
    Index() = default;

    static auto make(Geometry geometry) -> Index;

    static auto load(Geometry geometry, Vec<u8> bytes, u64 slots) -> Result<Index, Error>;

    static auto load_plain(Geometry geometry, Vec<u8> bytes) -> Result<Index, Error>;

    static auto load_segments(Geometry geometry, Vec<u8> bytes, u64 size) -> Result<Index, Error>;

    auto clone() const -> Index;

    auto get(u64 index) const -> Option<tuple<u32, array<u8, 32>>>;

    auto set(u64 index, u32 slot, const Bytes& data) -> Result<empty, Error>;

    void remove(u64 index);

    auto complete() const -> bool;

    auto dirty() -> Vec<Range>;

    auto bytes() const -> slice<u8> { return bytes_.as_slice(); }
    void committed() { dirty_.clear(); }
    auto is_committed() const -> bool { return dirty_.is_empty(); }
};
} // namespace qcm::backend::resource_cache
