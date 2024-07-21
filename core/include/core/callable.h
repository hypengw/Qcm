#pragma once
#include "core.h"
#include <functional>
#include <type_traits>
#include <concepts>

namespace ycore
{
namespace detail
{

using unique_void = std::unique_ptr<void, void (*)(const void*)>;
inline static unique_void make_unique_void_nullptr() {
    return unique_void(nullptr, [](const void*) {
    });
}

template<class R, class... Arguments>
struct handler_traits {
    template<typename Callable>
    struct handler {
        static void deletor(const void* void_cb) {
            if (void_cb != nullptr) {
                auto cb = static_cast<const Callable*>(void_cb);
                delete cb;
            }
        }

        template<typename... Args>
        static void create(unique_void& storage, Args&&... args) {
            storage = unique_void(new Callable(std::forward<Args>(args)...), handler::deletor);
        }

        static R call(unique_void& storage, Arguments... args) {
            return std::invoke(*static_cast<Callable*>(storage.get()),
                               std::forward<Arguments>(args)...);
        }
    };
};

template<typename R, typename... Arguments>
class callable_impl : NoCopy {
    template<typename T>
    using handler_type = typename handler_traits<R, Arguments...>::template handler<T>;

    using call_func = R (*)(unique_void&, Arguments...);

public:
    callable_impl() noexcept: m_storage(make_unique_void_nullptr()), m_call(nullptr) {}
    callable_impl(std::nullptr_t) noexcept: callable_impl() {}

    template<typename Callable, typename... Args>
    void create(Args&&... args) {
        using handler = handler_type<Callable>;
        handler::create(m_storage, std::forward<Args>(args)...);
        m_call = handler::call;
    }

    R call(Arguments... args) { return m_call(m_storage, std::forward<Arguments>(args)...); }

    explicit operator bool() const noexcept { return m_storage != nullptr; }

    callable_impl(callable_impl&& o) noexcept: m_call(std::exchange(o.m_call, nullptr)) {
        m_storage.swap(o.m_storage);
    }

    callable_impl& operator=(callable_impl&& o) noexcept {
        m_storage.swap(o.m_storage);
        m_call = std::exchange(o.m_call, nullptr);
        return *this;
    }

private:
    unique_void m_storage;
    call_func   m_call;
};
} // namespace detail

template<typename T>
class callable;

template<typename R, typename... Arguments>
class callable<R(Arguments...)> : public detail::callable_impl<R, Arguments...> {
    using base_type = detail::callable_impl<R, Arguments...>;

public:
    using base_type::base_type;

    template<typename F>
    callable(F&& f) requires std::invocable<F, Arguments...> {
        base_type::template create<std::decay_t<F>>(std::forward<F>(f));
    }

    template<typename F>
    callable& operator=(F&& f) requires std::invocable<F, Arguments...> {
        base_type::template create<std::decay_t<F>>(std::forward<F>(f));
        return *this;
    }

    R operator()(Arguments... args) { return base_type::call(std::forward<Arguments>(args)...); }
};

} // namespace ycore
