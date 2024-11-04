#pragma once

#pragma once

#include <QQmlEngine>

#include "Qcm/query/query.h"
#include "Qcm/query/query_model.h"
#include "Qcm/sql/item_sql.h"
#include "Qcm/app.h"
#include "asio_qt/qt_sql.h"
#include "qcm_interface/global.h"
#include "qcm_interface/macro.h"
#include "qcm_interface/async.inl"

namespace qcm::query
{

class MixDelete : public QObject {
    Q_OBJECT
public:
    MixDelete(QObject* parent = nullptr): QObject(parent) {}
};

class MixDeleteQuery : public Query<MixDelete> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(std::vector<model::ItemId> ids READ ids WRITE setIds NOTIFY idsChanged FINAL)
public:
    MixDeleteQuery(QObject* parent = nullptr): Query<MixDelete>(parent) {
        connect_requet_reload(&MixDeleteQuery::idsChanged);
    }

    auto ids() const { return m_ids; }
    void setIds(const std::vector<model::ItemId>& ids) {
        m_ids = ids;
        idsChanged();
    }

    Q_SIGNAL void idsChanged();

public:
    void reload() override {
        set_status(Status::Querying);
        auto ex   = asio::make_strand(pool_executor());
        auto self = helper::QWatcher { this };
        auto c    = Global::instance()->qsession()->client();
        if (! c) return;
        spawn(ex, [self, c = c.value(), ids = m_ids] -> task<void> {
            auto out = co_await c.api->delete_mix(c, ids);
            co_await asio::post(
                asio::bind_executor(Global::instance()->qexecutor(), asio::use_awaitable));
            if (self) {
                if (out) {
                    self->set_status(Status::Finished);
                } else {
                    self->set_error(convert_from<QString>(out.error().what()));
                    self->set_status(Status::Error);
                }
            }
            co_return;
        });
    }

private:
    std::vector<model::ItemId> m_ids;
};

} // namespace qcm::query
