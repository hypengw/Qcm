module;
#include <rstd/macro.hpp>
#include "Qcm/macro.hpp"
module qcm;
import :util.global_static;

namespace qcm
{
struct GlobalStatic::HolderImpl {
    cppstd::atomic<voidp>         instance { nullptr };
    cppstd::function<void(voidp)> deleter;

    void reset() {
        if (deleter && instance) {
            deleter(instance);
            deleter  = {};
            instance = nullptr;
        }
    }

    HolderImpl() {}
    ~HolderImpl() { reset(); }
};

class GlobalStatic::Private {
public:
    cppstd::map<cppstd::string, rc<HolderImpl>, cppstd::less<>> instances;
};

GlobalStatic::GlobalStatic(): d_ptr(make_up<Private>()) {}
GlobalStatic::~GlobalStatic() { reset(); }
auto GlobalStatic::instance() -> GlobalStatic* {
    static GlobalStatic theGlobalStatic;
    return &theGlobalStatic;
}

auto GlobalStatic::data(HolderImpl& holder) -> voidp {
    assert(holder.instance);
    return holder.instance;
}

auto GlobalStatic::add_impl(cppstd::string_view name, voidp instance,
                            cppstd::function<void(voidp)> deleter) -> rc<HolderImpl> {
    C_D(GlobalStatic);
    auto holder      = make_rc<HolderImpl>();
    holder->instance = instance;
    holder->deleter  = deleter;
    d->instances.insert({ cppstd::string(name), holder });
    return holder;
}
void GlobalStatic::reset() {
    C_D(GlobalStatic);
    for (auto& el : d->instances) {
        el.second->reset();
    }
    d->instances.clear();
}
} // namespace qcm