#pragma once

#pragma once

#include <QQmlEngine>

#include "qcm_interface/query.h"
#include "Qcm/query/query_load.h"
#include "Qcm/sql/item_sql.h"
#include "Qcm/app.hpp"
#include "core/qasio/qt_sql.h"
#include "Qcm/global.hpp"
#include "Qcm/macro.hpp"
#include "Qcm/util/async.inl"

namespace qcm
{

class MixManipulate : public QObject {
    Q_OBJECT
public:
    MixManipulate(QObject* parent = nullptr): QObject(parent) {}
};

class MixManipulateQuery : public Query, public QueryExtra<MixManipulate> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qcm::model::ItemId mixId READ itemId WRITE setItemId NOTIFY itemIdChanged FINAL)
    Q_PROPERTY(std::vector<model::ItemId> itemIds READ ids WRITE setIds NOTIFY idsChanged FINAL)
    Q_PROPERTY(qint32 oper READ oper WRITE setOper NOTIFY operChanged FINAL)
public:
    MixManipulateQuery(QObject* parent = nullptr): Query, public QueryExtra<MixManipulate>(parent), m_oper(0) {
        // connect_requet_reload(&MixManipulateidsChanged);
        connect(this, &MixManipulatestatusChanged, [this](Status s) {
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
            auto sql = App::instance()->item_sql();
            co_await sql->insert(std::array { pl }, {});
        }
    }

    void reload() override {
        setStatus(Status::Querying);
        auto self = helper::QWatcher { this };
        auto c    = Global::instance()->qsession()->client();
        if (! c) return;
        spawn( [self, c = c.value(), id = m_id, ids = m_ids, oper = m_oper] -> task<void> {
            auto out = co_await c.api->manipulate_mix(c, id, (enums::ManipulateMixAction)oper, ids);
            co_await asio::post(
                asio::bind_executor(qcm::qexecutor(), asio::use_awaitable));
            if (self) {
                if (out) {
                    self->setStatus(Status::Finished);
                } else {
                    self->setError(convert_from<QString>(out.error().what()));
                    self->setStatus(Status::Error);
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

} // namespace qcm
