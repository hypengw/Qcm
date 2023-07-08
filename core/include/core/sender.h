#pragma once
#include <future>
#include "core/core.h"

namespace qcm
{

namespace detail
{

template<typename T>
class Sender {
public:
    Sender()                 = default;
    virtual ~Sender()        = default;
    virtual bool try_send(T) = 0;
    virtual std::future<void> send(T) = 0;
    virtual void reset()     = 0;
};

} // namespace detail

template<typename T>
class Sender {
public:
    using Inner = detail::Sender<T>;
    Sender()    = default;
    ~Sender()   = default;
    Sender(rc<Inner> in): m_impl(in) {}

    bool try_send(T t) { return m_impl->try_send(t); }
    std::future<void> send(T t) { return m_impl->send(t); }
    void reset() { m_impl->reset(); }

private:
    rc<Inner> m_impl;
};

} // namespace qcm
