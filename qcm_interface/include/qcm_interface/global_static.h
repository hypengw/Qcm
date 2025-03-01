#pragma once

#include <functional>
#include <atomic>
#include <string_view>
#include "qcm_interface/export.h"

namespace qcm
{
class Global;
class GlobalStatic {
    friend class Global;

public:
    QCM_INTERFACE_API GlobalStatic();
    QCM_INTERFACE_API ~GlobalStatic();
    QCM_INTERFACE_API static auto instance() -> GlobalStatic*;

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

private:
    QCM_INTERFACE_API auto        add_impl(std::string_view name, voidp instance,
                                           std::function<void(voidp)> deleter) -> rc<HolderImpl>;
    QCM_INTERFACE_API static auto data(HolderImpl&) -> voidp;

    void reset();

    class Private;
    C_DECLARE_PRIVATE(GlobalStatic, d_ptr);
};
} // namespace qcm