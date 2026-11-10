export module qcm.backend:pagination;
export import rstd;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct Page {
    i32 size {};
    i64 offset {};

    auto more(i64 total) const -> bool {
        return offset + i64(static_cast<long long>(size.to_primitive())) < total;
    }
};

auto page_of(i32 page, i32 size) -> Page {
    auto fixed_size = size > i32() ? size : i32(1);
    auto fixed_page = page > i32() ? page : i32();
    auto offset     = i64(static_cast<long long>(fixed_page.to_primitive()) *
                          static_cast<long long>(fixed_size.to_primitive()));
    return { fixed_size, offset };
}
} // namespace qcm::backend
