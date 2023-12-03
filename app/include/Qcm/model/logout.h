#pragma once

#include <QQmlEngine>
#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/logout.h"

#include "core/log.h"

namespace qcm
{

namespace model
{

class Logout : public QObject {
    Q_OBJECT
public:
    Logout(QObject* parent = nullptr): QObject(parent) {}
    using out_type = ncm::api_model::Logout;

    READ_PROPERTY(qint32, code, m_code, infoChanged)

    void handle_output(const out_type& in, const auto&) {
        auto& o = *this;
        convert(o.m_code, in.code);
        emit infoChanged();
    }

signals:
    void infoChanged();
};
static_assert(modelable<Logout, ncm::api::Logout>);

} // namespace model

using LogoutQuerier_base = ApiQuerier<ncm::api::Logout, model::Logout>;
class LogoutQuerier : public LogoutQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    LogoutQuerier(QObject* parent = nullptr): LogoutQuerier_base(parent) {}
};
} // namespace qcm
