module;
#include <map>
#include <atomic>
#include <string>
#include <string_view>
#include <functional>
#include <atomic>
#include <string_view>

#include "core/log.h"
export module qcm.util.global_static;
export import qcm.core;

namespace qcm
{
export class GlobalStatic {
public:
    GlobalStatic();
    ~GlobalStatic();
    static auto instance() -> GlobalStatic*;

    struct HolderImpl;

    template<typename T>
    struct Holder {
        Holder(rc<HolderImpl> d): d_ptr(d) {}
        ~Holder() = default;
        auto operator->() -> HolderImpl* { return d_ptr.get(); };
             operator T*() { return static_cast<T*>(data(*d_ptr)); };

    private:
        rc<HolderImpl> d_ptr;
    };

    template<typename T>
    auto add(std::string_view name, T* instance, std::function<void(T*)> deleter) -> Holder<T> {
        return add_impl(name, static_cast<voidp>(instance), [deleter](voidp p) {
            deleter(static_cast<T*>(p));
        });
    }

    void reset();

private:
    auto add_impl(std::string_view name, voidp instance, std::function<void(voidp)> deleter)
        -> rc<HolderImpl>;
    static auto data(HolderImpl&) -> voidp;

    class Private;
    C_DECLARE_PRIVATE(GlobalStatic, d_ptr);
};
} // namespace qcm

module :private;
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