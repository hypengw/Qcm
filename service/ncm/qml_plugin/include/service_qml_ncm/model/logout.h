#pragma once

#include <QQmlEngine>
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/logout.h"

#include "core/log.h"

namespace ncm::qml
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

    Q_SIGNAL void infoChanged();
};

} // namespace model

using LogoutQuerier_base = NcmApiQuery<ncm::api::Logout, model::Logout>;
class LogoutQuerier : public LogoutQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    LogoutQuerier(QObject* parent = nullptr): LogoutQuerier_base(parent) {}
};
} // namespace ncm::qml
