#include "Qcm/util/global_static.hpp"

#include <map>
#include <atomic>

#include "core/log.h"

namespace qcm
{
struct GlobalStatic::HolderImpl {
    std::atomic<voidp>         instance { nullptr };
    std::function<void(voidp)> deleter;

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
    std::map<std::string, rc<HolderImpl>, std::less<>> instances;
};

GlobalStatic::GlobalStatic(): d_ptr(make_up<Private>()) {}
GlobalStatic::~GlobalStatic() { reset(); }
auto GlobalStatic::instance() -> GlobalStatic* {
    static GlobalStatic theGlobalStatic;
    return &theGlobalStatic;
}

auto GlobalStatic::data(HolderImpl& holder) -> voidp {
    _assert_rel_(holder.instance);
    return holder.instance;
}

auto GlobalStatic::add_impl(std::string_view name, voidp instance,
                            std::function<void(voidp)> deleter) -> rc<HolderImpl> {
    C_D(GlobalStatic);
    auto holder      = make_rc<HolderImpl>();
    holder->instance = instance;
    holder->deleter  = deleter;
    d->instances.insert({ std::string(name), holder });
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