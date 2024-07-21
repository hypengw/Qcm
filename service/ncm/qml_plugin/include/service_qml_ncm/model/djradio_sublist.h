#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>
#include <set>

#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/djradio_sublist.h"

#include "meta_model/qgadgetlistmodel.h"

#include "core/log.h"

namespace qcm
{
namespace model
{

class DjradioSublist : public meta_model::QGadgetListModel<Djradio> {
    Q_OBJECT
public:
    DjradioSublist(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<Djradio>(parent), m_has_more(true) {}
    using out_type = ncm::api_model::DjradioSublist;

    void handle_output(const out_type& re, const auto& input) {
        if (input.offset == 0) {
            auto in_ = convert_from<std::vector<Djradio>>(re.djRadios);
            convertModel(in_, [](const Djradio& it) -> std::string {
                return convert_from<std::string>(it.id);
            });
            m_has_more = re.hasMore;
        } else if (input.offset == (int)rowCount()) {
            if (! re.djRadios.empty()) {
                insert(rowCount(), convert_from<std::vector<Djradio>>(re.djRadios));
            }
            m_has_more = re.hasMore;
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
static_assert(modelable<DjradioSublist, ncm::api::DjradioSublist>);
} // namespace model

using DjradioSublistQuerier_base = ApiQuerier<ncm::api::DjradioSublist, model::DjradioSublist>;
class DjradioSublistQuerier : public DjradioSublistQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    DjradioSublistQuerier(QObject* parent = nullptr): DjradioSublistQuerier_base(parent) {}

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
