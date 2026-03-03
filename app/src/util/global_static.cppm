module;
#include "core/log.h"
export module qcm:util.global_static;
export import qcm.core;

namespace cppstd = rstd::cppstd;
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
    auto add(cppstd::string_view name, T* instance, cppstd::function<void(T*)> deleter) -> Holder<T> {
        return add_impl(name, static_cast<voidp>(instance), [deleter](voidp p) {
            deleter(static_cast<T*>(p));
        });
    }

    void reset();

private:
    auto add_impl(cppstd::string_view name, voidp instance, cppstd::function<void(voidp)> deleter)
        -> rc<HolderImpl>;
    static auto data(HolderImpl&) -> voidp;

    class Private;
    C_DECLARE_PRIVATE(GlobalStatic, d_ptr);
};
} // namespace qcm
