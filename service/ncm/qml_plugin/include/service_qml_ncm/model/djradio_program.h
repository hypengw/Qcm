#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>
#include <set>

#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/djradio_program.h"

#include "meta_model/qgadgetlistmodel.h"

#include "core/log.h"

namespace qcm
{
namespace model
{

class DjradioProgram : public meta_model::QGadgetListModel<Program> {
    Q_OBJECT
public:
    DjradioProgram(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<Program>(parent), m_has_more(true) {}
    using out_type = ncm::api_model::DjradioProgram;

    void handle_output(const out_type& re, const auto& input) {
        if (input.offset == (int)rowCount()) {
            if (! re.programs.empty()) {
                insert(rowCount(), convert_from<std::vector<Program>>(re.programs));
            }
            m_has_more = re.more;
        }
    }

    bool canFetchMore(const QModelIndex&) const override { return m_has_more; }
    void fetchMore(const QModelIndex&) override {
        m_has_more = false;
        emit fetchMoreReq(rowCount());
    }
signals:
    void fetchMoreReq(qint32);

private:
    bool m_has_more;
};
static_assert(modelable<DjradioProgram, ncm::api::DjradioProgram>);
} // namespace model

using DjradioProgramQuerier_base = ApiQuerier<ncm::api::DjradioProgram, model::DjradioProgram>;
class DjradioProgramQuerier : public DjradioProgramQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    DjradioProgramQuerier(QObject* parent = nullptr): DjradioProgramQuerier_base(parent) {}

    FORWARD_PROPERTY(model::DjradioId, itemId, radioId)
    FORWARD_PROPERTY(qint32, offset, offset)
    FORWARD_PROPERTY(qint32, limit, limit)

public:
    void fetch_more(qint32 cur_count) override { set_offset(cur_count); }
public slots:
    void reset() {
        api().input.offset = 0;
        reload();
    }
};

} // namespace qcm
