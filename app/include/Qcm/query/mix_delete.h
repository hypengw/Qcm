#pragma once

#pragma once

#include <QQmlEngine>

#include "qcm_interface/query.h"
#include "Qcm/query/query_model.h"
#include "Qcm/sql/item_sql.h"
#include "Qcm/app.h"
#include "asio_qt/qt_sql.h"
#include "qcm_interface/global.h"
#include "qcm_interface/macro.h"
#include "qcm_interface/async.inl"
#include "Qcm/sql/collection_sql.h"

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

    Q_PROPERTY(std::vector<model::ItemId> itemIds READ ids WRITE setIds NOTIFY idsChanged FINAL)
public:
    MixDeleteQuery(QObject* parent = nullptr): Query<MixDelete>(parent) {}

    auto ids() const { return m_ids; }
    void setIds(const std::vector<model::ItemId>& ids) {
        m_ids = ids;
        idsChanged();
    }

    Q_SIGNAL void idsChanged();

public:
    auto mix_delete(const model::ItemId&           user_id,
                    std::span<const model::ItemId> ids) -> task<void> {
        auto sql = App::instance()->collect_sql();
        co_await sql->remove(user_id, ids);
    }

    void reload() override {
        set_status(Status::Querying);
        auto self    = helper::QWatcher { this };
        auto user_id = Global::instance()->qsession()->user()->userId();
        auto c       = Global::instance()->qsession()->client();
        if (! c) return;
        spawn( [self, c = c.value(), ids = m_ids, user_id] -> task<void> {
            auto out = co_await c.api->delete_mix(c, ids);
            if (out) {
                co_await self->mix_delete(user_id, ids);
            }
            co_await asio::post(
                asio::bind_executor(Global::instance()->qexecutor(), asio::use_awaitable));
            if (self) {
                if (out) {
                    self->set_status(Status::Finished);
                    for (auto& el : ids) {
                        Notifier::instance()->collected(el, false);
                    }
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
