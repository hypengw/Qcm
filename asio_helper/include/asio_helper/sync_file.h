#pragma once
#include <fstream>
#include <utility>

#include <asio/buffer.hpp>

#include "core/core.h"

namespace helper
{

template<typename F>
class SyncFile : NoCopy {
public:
    SyncFile(F&& f): m_f(std::forward<F>(f)) {}

    SyncFile(SyncFile&& o) noexcept: m_f(std::exchange(o.m_f, {})) {}
    SyncFile& operator=(SyncFile&& o) noexcept {
        m_f = std::exchange(o.m_f, {});
        return *this;
    }

    F& handle() { return m_f; }

    template<typename MB>
        requires asio::is_const_buffer_sequence<MB>::value
    auto write_some(const MB& buffer) {
        auto size = buffer.size();
        m_f.write((const char*)buffer.data(), size);
        return size;
    }

private:
    F m_f;
};

} // namespace helper
