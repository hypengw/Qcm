export module qcm.backend:db.provider.item;
export import :error;
export import :model.item;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct ProviderItem {
    i64      id {};
    i64      provider_id {};
    String   native_id;
    ItemType type { ItemType::Unspecified };
};
struct ProviderItemRead {
private:
    i64      id_;
    ItemType type_;

public:
    ProviderItemRead(i64 id, ItemType type): id_(id), type_(type) {}

    auto operator()(sqlito::Connection& db) const -> ProcessResult<Option<ProviderItem>>;
};
} // namespace qcm::backend
