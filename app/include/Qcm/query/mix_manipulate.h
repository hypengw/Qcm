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

class MixManipulate : public QObject {
    Q_OBJECT
public:
    MixManipulate(QObject* parent = nullptr): QObject(parent) {}
};

class MixManipulateQuery : public Query<MixManipulate> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(model::ItemId mixId READ itemId WRITE setItemId NOTIFY itemIdChanged FINAL)
    Q_PROPERTY(std::vector<model::ItemId> itemIds READ ids WRITE setIds NOTIFY idsChanged FINAL)
    Q_PROPERTY(qint32 oper READ oper WRITE setOper NOTIFY operChanged FINAL)
public:
    MixManipulateQuery(QObject* parent = nullptr): Query<MixManipulate>(parent), m_oper(0) {
        // connect_requet_reload(&MixManipulateQuery::idsChanged);
        connect(this, &MixManipulateQuery::statusChanged, [this](Status s) {
            if (s == Status::Finished) {
                Action::instance()->sync_item(this->itemId(), true);
            }
        });
    }

    auto itemId() const -> const model::ItemId& { return m_id; }
    void setItemId(const model::ItemId& id) {
        if (ycore::cmp_exchange(m_id, id)) {
            itemIdChanged();
        }
    }
    Q_SIGNAL void itemIdChanged();

    auto ids() const { return m_ids; }
    void setIds(const std::vector<model::ItemId>& ids) {
        m_ids = ids;
        idsChanged();
    }
    Q_SIGNAL void idsChanged();

    auto oper() const { return m_oper; }
    void setOper(qint32 oper) {
        if (ycore::cmp_exchange(m_oper, oper)) {
            operChanged();
        }
    }
    Q_SIGNAL void operChanged();

public:
    auto mix_manipulate(const model::Mix& pl) -> task<void> {
        {
            auto sql = App::instance()->album_sql();
            co_await sql->insert(std::array { pl }, {});
        }
    }

    void reload() override {
        set_status(Status::Querying);
        auto ex   = asio::make_strand(pool_executor());
        auto self = helper::QWatcher { this };
        auto c    = Global::instance()->qsession()->client();
        if (! c) return;
        spawn(ex, [self, c = c.value(), id = m_id, ids = m_ids, oper = m_oper] -> task<void> {
            auto out = co_await c.api->manipulate_mix(c, id, (enums::ManipulateMixAction)oper, ids);
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
    model::ItemId              m_id;
    std::vector<model::ItemId> m_ids;
    qint32                     m_oper;
};

} // namespace qcm::query
