#pragma once

#include <utility>

namespace core 
{

template <class F>
struct y_combinator {
    F f; // the lambda will be stored here
    
    // a forwarding operator():
    template <class... Args>
    decltype(auto) operator()(Args&&... args) const {
        // we pass ourselves to f, then the arguments.
        return f(*this, std::forward<Args>(args)...);
    }
};

template <class F>
y_combinator(F) -> y_combinator<F>;

}