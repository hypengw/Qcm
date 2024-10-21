#pragma once
#include "qcm_interface/async.h"

namespace qcm::query
{

template<typename T>
class Query : public QAsyncResult {
public:
    Query(QObject* parent = nullptr): QAsyncResult(parent), m_data(new T(this)) {}
    ~Query() {}

    auto data() const -> QObject* override { return m_data; }
    auto tdata() const -> T* { return m_data; }

private:
    T* m_data;
};
} // namespace qcm::query