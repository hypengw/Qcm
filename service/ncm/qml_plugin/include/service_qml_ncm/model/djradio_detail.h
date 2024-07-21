#pragma once

#include <QQmlEngine>
#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/djradio_detail.h"

#include "core/log.h"

namespace qcm
{

namespace model
{

class DjradioDetail : public QObject {
    Q_OBJECT
public:
    DjradioDetail(QObject* parent = nullptr): QObject(parent) {}

    READ_PROPERTY(QString, name, m_name, infoChanged)
    READ_PROPERTY(QString, picUrl, m_picUrl, infoChanged)
    READ_PROPERTY(QString, description, m_description, infoChanged)
    READ_PROPERTY(qint32, programCount, m_programCount, infoChanged)
    READ_PROPERTY(QDateTime, createTime, m_createTime, infoChanged)
    READ_PROPERTY(QDateTime, lastProgramTime, m_lastProgramTime, infoChanged)
    READ_PROPERTY(bool, subed, m_subed, infoChanged)

    using out_type = ncm::api_model::DjradioDetail;

    void handle_output(const out_type& in_, const auto&) {
        auto& in = in_.data;
        auto& o  = *this;
        convert(o.m_name, in.name);
        convert(o.m_picUrl, in.picUrl);
        convert(o.m_description, in.desc);
        convert(o.m_programCount, in.programCount);
        convert(o.m_createTime, in.createTime);
        convert(o.m_lastProgramTime, in.lastProgramCreateTime);
        convert(o.m_subed, in.subed);
        emit infoChanged();
    }

signals:
    void infoChanged();
};
static_assert(modelable<DjradioDetail, ncm::api::DjradioDetail>);

} // namespace model

using DjradioDetailQuerier_base = ApiQuerier<ncm::api::DjradioDetail, model::DjradioDetail>;
class DjradioDetailQuerier : public DjradioDetailQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    DjradioDetailQuerier(QObject* parent = nullptr): DjradioDetailQuerier_base(parent) {}

    FORWARD_PROPERTY(model::ItemId, itemId, id)
};

} // namespace qcm
