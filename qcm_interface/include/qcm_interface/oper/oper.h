#pragma once

#include <stdexcept>
#include <ranges>
#include "qcm_interface/export.h"

namespace qcm::oper
{

template<typename T>
class OperList {
public:
    using Holder      = std::unique_ptr<void, void (*)(voidp)>;
    using DataOp      = T* (*)(voidp);
    using SizeOp      = usize (*)(voidp);
    using Advance     = T* (*)(T*, usize);
    using EmplaceBack = T* (*)(voidp);

    OperList(): m_data(nullptr), m_size(nullptr), m_adv(nullptr), m_eb(nullptr) {}
    OperList(Holder&& h, DataOp data, SizeOp size, Advance adv, EmplaceBack eb)
        : m_holder(std::move(h)), m_data(data), m_size(size), m_adv(adv), m_eb(eb) {}
    ~OperList()                          = default;
    OperList(const OperList&)            = delete;
    OperList& operator=(const OperList&) = delete;
    OperList(OperList&& o): OperList() { *this = o; }
    OperList& operator=(OperList&& o) {
        m_holder.swap(o.m_holder);
        std::swap(m_data, o.m_data);
        std::swap(m_size, o.m_size);
        std::swap(m_adv, o.m_adv);
    }

    auto operator[](usize i) const noexcept -> const T* { return m_adv(data(), i); }
    auto operator[](usize i) noexcept -> T* { return m_adv(data(), i); }
    auto at(usize i) const -> T* {
        if (i >= size()) throw std::out_of_range("OperList");
        return m_adv(data(), i);
    }

    auto data() const noexcept -> const T* {
        return m_data == nullptr ? 0 : m_data(m_holder.get());
    }
    auto data() noexcept -> T* { return m_data == nullptr ? 0 : m_data(m_holder.get()); }
    auto size() const noexcept -> usize { return m_size == nullptr ? 0 : m_size(m_holder.get()); }

    auto emplace_back() -> T* { return m_eb(m_holder.get()); }

    operator std::span<const T>() const { return { data(), size() }; }

    auto to_view() {
        return std::views::transform(std::views::iota(0u, size()), [this](auto i) {
            return at(i);
        });
    }

private:
    Holder      m_holder;
    DataOp      m_data;
    SizeOp      m_size;
    Advance     m_adv;
    EmplaceBack m_eb;
};

template<typename T>
struct QCM_INTERFACE_API Oper {
    Oper(T* m): model(m) {}
    ~Oper() = default;

    operator T*() { return model; }

    static auto create_list(usize num) -> OperList<T>;
    T*          model;
};

} // namespace qcm::oper

#define OPER_PROPERTY(type, prop, mem) \
    auto mem() -> const type&;         \
    void set_##mem(const type&);

#define IMPL_OPER_PROPERTY(class_, type, prop, mem)                \
    auto class_::mem() -> const type& { return this->model->mem; } \
    void class_::set_##mem(const type& v) { this->model->mem = v; }
