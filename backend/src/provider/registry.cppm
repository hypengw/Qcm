export module qcm.backend:provider.registry;
export import rstd;
export import :provider.factory;
using namespace rstd::prelude;
using namespace rstd::literals;

export namespace qcm::backend
{
struct ProviderMetaInfo {
    String             type_name;
    String             svg;
    bool               has_server_url { true };
    bool               is_mutable {};
    bool               is_script {};
    Vec<i32>           auth_types;
    ProviderSyncConfig sync;
    ProviderFactory    creator;

    auto clone() const -> ProviderMetaInfo {
        return { type_name.clone(), svg.clone(),        has_server_url, is_mutable,
                 is_script,         auth_types.clone(), sync.clone(),   creator.clone() };
    }
};
class ProviderRegistry {
    Vec<ProviderMetaInfo> metas_;

public:
    auto metas() const -> slice<ProviderMetaInfo> { return metas_.as_slice(); }
    auto find(ref<str> type_name) const -> const ProviderMetaInfo* {
        for (const auto& meta : metas_)
            if (meta.type_name == type_name) return rstd::addressof(meta);
        return nullptr;
    }
    auto register_provider(ProviderMetaInfo meta) -> ProviderResult<empty> {
        if (meta.type_name.is_empty() || ! meta.creator)
            return Err(
                ProviderError::internal("provider registration requires a type and creator"_str));
        if (find(meta.type_name.as_str()) != nullptr)
            return Err(ProviderError::internal("duplicate provider type"_str));
        metas_.push(rstd::move(meta));
        return Ok(empty {});
    }
    auto create(ProviderOptions options) const -> ProviderResult<ProviderHandle> {
        auto* meta = find(options.type_name.as_str());
        if (meta == nullptr) return Err(ProviderError::NotFound());
        return meta->creator.create(rstd::move(options));
    }
};
} // namespace qcm::backend
