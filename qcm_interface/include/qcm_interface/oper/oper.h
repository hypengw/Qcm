#pragma once

#include <stdexcept>
#include "qcm_interface/export.h"

namespace qcm::oper
{

template<typename T>
class OperList {
public:
    using Holder  = std::unique_ptr<void, void (*)(voidp)>;
    using Advance = T* (*)(T*, usize);

    OperList(): m_data(nullptr), m_size(0), m_adv(nullptr) {}
    OperList(Holder&& h, T* data, usize size, Advance adv)
        : m_holder(std::move(h)), m_data(data), m_size(size), m_adv(adv) {}
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

    auto operator[](usize i) const noexcept -> T* { return m_adv(m_data, i); }
    auto at(usize i) const -> T* {
        if (i >= m_size) throw std::out_of_range("OperList");
        return m_adv(m_data, i);
    }

    auto data() const noexcept -> const T* { return m_data; }
    auto size() const noexcept -> usize { return m_size; }

private:
    Holder  m_holder;
    T*      m_data;
    usize   m_size;
    Advance m_adv;
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
