#pragma once

#include <utility>

namespace ycore
{

template<class F>
struct y_combinator {
    F f; // the lambda will be stored here

    // a forwarding operator():
    template<class... Args>
    decltype(auto) operator()(Args&&... args) const {
        // we pass ourselves to f, then the arguments.
        return f(*this, std::forward<Args>(args)...);
    }
};

template<class F>
y_combinator(F) -> y_combinator<F>;

template<typename R, typename... Args>
struct function_traits_base {
    using ret_type                         = R;
    using arg_types                        = std::tuple<Args...>;
    static constexpr std::size_t arg_count = sizeof...(Args);
    template<std::size_t N>
    using nth_arg = std::tuple_element_t<N, arg_types>;
};

template<typename F>
struct function_traits;

template<typename R, typename... Args>
struct function_traits<R (*)(Args...)> : function_traits_base<R, Args...> {
    using pointer = R (*)(Args...);
};

} // namespace ycore