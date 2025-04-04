#pragma once

#pragma once

#include <QQmlEngine>

#include "qcm_interface/query.h"
#include "Qcm/query/query_load.h"
#include "Qcm/sql/item_sql.h"
#include "Qcm/app.hpp"
#include "Qcm/sql/collection_sql.h"
#include "core/qasio/qt_sql.h"
#include "Qcm/global.hpp"
#include "Qcm/macro.hpp"
#include "Qcm/util/async.inl"

namespace qcm::query
{

class MixCreate : public QObject {
    Q_OBJECT
public:
    MixCreate(QObject* parent = nullptr): QObject(parent) {}
};

class MixCreateQuery : public Query<MixCreate> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged FINAL)
public:
    MixCreateQuery(QObject* parent = nullptr): Query<MixCreate>(parent) {}

    auto name() const -> const QString& { return m_name; }
    void setName(const QString& v) {
        if (ycore::cmp_exchange(m_name, v)) {
            nameChanged();
        }
    }

    Q_SIGNAL void nameChanged();

public:
    auto mix_create(const model::Mix& pl) -> task<void> {
        {
            auto sql = App::instance()->item_sql();
            co_await sql->insert(std::array { pl }, {});
        }
        {
            auto sql = App::instance()->collect_sql();
            co_await sql->insert(std::array { CollectionSql::Item::from(pl.userId, pl.id) });
        }
    }

    void reload() override {
        set_status(Status::Querying);
        auto self   = helper::QWatcher { this };
        auto c      = Global::instance()->qsession()->client();
        auto userId = Global::instance()->qsession()->user()->userId();
        if (! c) return;
        spawn( [self, c = c.value(), name = m_name, userId] -> task<void> {
            model::Mix pl;
            auto            out = co_await c.api->create_mix(c, name);
            if (out) {
                pl.id     = out.value();
                pl.userId = userId;
                pl.name   = name;

                co_await self->mix_create(pl);
            }

            co_await asio::post(
                asio::bind_executor(qcm::qexecutor(), asio::use_awaitable));
            if (self) {
                if (out) {
                    self->set_status(Status::Finished);
                    Notifier::instance()->collected(pl.id, true);
                } else {
                    self->set_error(convert_from<QString>(out.error().what()));
                    self->set_status(Status::Error);
                }
            }
            co_return;
        });
    }

private:
    QString m_name;
};

} // namespace qcm::query
