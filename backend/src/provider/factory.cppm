export module qcm.backend:provider.factory;
export import :provider.interface;
export import :provider.dto;
export import :playing;
export import :plugin.config;
export import :plugin.context;
export import rstd.json;
using alloc::rc::make_rc;
using alloc::rc::Rc;
using rstd::async::RuntimeHandle;
using rstd::sync::Arc;
using namespace rstd::prelude;

export namespace qcm::backend
{
struct ProviderOptions {
    Option<RuntimeHandle> runtime;
    String                type_name;
    String                name;
    String                base_url;
    String                device_id;
    PluginConfig          config;
    Option<PluginContext> installation;
    Option<i64>           id;
    Arc<PlayingState>     playing { Arc<PlayingState>::make() };
    Option<AuthMethod>    auth_method;
    SettingsSnapshot      settings;
};

class ProviderFactory {
    using Creator = Box<dyn<Fn<ProviderResult<ProviderHandle>(ProviderOptions)>>>;
    Rc<Creator> creator_;
    explicit ProviderFactory(Rc<Creator> creator): creator_(rstd::move(creator)) {}

public:
    ProviderFactory() = default;
    template<class F>
    static auto make(F creator) -> ProviderFactory {
        return ProviderFactory(make_rc<Creator>(Creator::make(rstd::move(creator))));
    }
    auto     clone() const -> ProviderFactory { return ProviderFactory(creator_.clone()); }
    explicit operator bool() const { return bool(creator_); }
    auto     create(ProviderOptions options) const -> ProviderResult<ProviderHandle> {
        if (! creator_) return Err(ProviderError::NotImplemented());
        auto creator = creator_.clone();
        return (*creator)->operator()(rstd::move(options));
    }
};
} // namespace qcm::backend
