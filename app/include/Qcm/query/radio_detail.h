#pragma once

#pragma once

#include <QQmlEngine>

#include "qcm_interface/query.h"
#include "Qcm/query/query_model.h"
#include "Qcm/sql/item_sql.h"
#include "Qcm/app.h"
#include "asio_qt/qt_sql.h"
#include "qcm_interface/global.h"
#include "meta_model/qgadgetlistmodel.h"
#include "qcm_interface/macro.h"
#include "qcm_interface/async.inl"

namespace qcm::query
{

class RadioDetail : public meta_model::QGadgetListModel<Program> {
    Q_OBJECT

    Q_PROPERTY(Radio info READ info NOTIFY infoChanged)

    using base_type = meta_model::QGadgetListModel<Program>;

public:
    RadioDetail(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<Program>(parent), m_has_more(true) {}

    auto info() const -> const Radio& { return m_info; }
    void setInfo(const std::optional<Radio>& v) {
        m_info = v.value_or(Radio {});
        infoChanged();
    }

    bool canFetchMore(const QModelIndex&) const override { return m_has_more; }
    void fetchMore(const QModelIndex&) override {
        m_has_more = false;
        emit fetchMoreReq(rowCount());
    }

    Q_SIGNAL void fetchMoreReq(qint32);
    Q_SIGNAL void infoChanged();

    auto toSong(const Program& program) const -> Song {
        Song s;
        s.id           = program.songId;
        s.name         = program.name;
        s.coverUrl     = program.coverUrl;
        s.album.name   = m_info.name;
        s.album.picUrl = m_info.picUrl;
        s.canPlay      = true;
        s.duration     = program.duration;
        s.sourceId     = m_info.id;
        s.trackNumber  = program.serialNumber;
        return s;
    }

    QHash<int, QByteArray> roleNames() const override {
        auto rn = base_type::roleNames();
        rn.insert(10, "song");
        return rn;
    }
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (role == 10) {
            return QVariant::fromValue(song(index.row()));
        }
        return base_type::data(index, role);
    };

    Q_INVOKABLE Song song(qint32 idx) const {
        Song out;
        if (idx >= 0 && idx < rowCount()) {
            const auto& program = at(idx);
            out                 = toSong(program);
        }
        return out;
    }

    Q_INVOKABLE std::vector<Song> collectSongs() const {
        std::vector<Song> out;
        for (auto i = 0; i < rowCount(); i++) {
            const auto& program = at(i);
            auto&       s       = out.emplace_back();
            s                   = toSong(program);
        }
        return out;
    }

private:
    bool  m_has_more;
    Radio m_info;
};

class RadioDetailQuery : public Query<RadioDetail> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
    Q_PROPERTY(RadioDetail* data READ tdata NOTIFY itemIdChanged FINAL)
public:
    RadioDetailQuery(QObject* parent = nullptr): Query<RadioDetail>(parent) {
        connect(this, &RadioDetailQuery::itemIdChanged, this, &RadioDetailQuery::reload);
    }

    auto itemId() const -> const model::ItemId& { return m_radio_id; }
    void setItemId(const model::ItemId& v) {
        if (ycore::cmp_exchange(m_radio_id, v)) {
            itemIdChanged();
        }
    }

    Q_SIGNAL void itemIdChanged();

public:
    auto query_radio(model::ItemId itemId) -> task<std::optional<Radio>> {
        auto sql = App::instance()->album_sql();
        co_await asio::post(asio::bind_executor(sql->get_executor(), use_task));

        auto query = sql->con()->query();
        query.prepare_sv(std::format(R"(
SELECT 
    {0}
FROM radio
WHERE radio.itemId = :itemId AND ({1})
GROUP BY radio.itemId;
)",
                                     model::Radio::sql().select,
                                     db::null<db::AND, db::NOT>(model::Radio::sql().columns)));
        query.bindValue(":itemId", itemId.toUrl());

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
        } else if (query.next()) {
            Radio dj;
            int   i = 0;
            load_query(query, dj, i);
            co_return dj;
        }
        co_return std::nullopt;
    }

    auto query_programs(model::ItemId itemId) -> task<std::optional<std::vector<Program>>> {
        auto sql = App::instance()->album_sql();
        co_await asio::post(asio::bind_executor(sql->get_executor(), use_task));
        auto query = sql->con()->query();
        query.prepare_sv(fmt::format(R"(
SELECT 
    {0}
FROM program
WHERE program.radioId = :itemId
GROUP BY program.itemId
ORDER BY program.serialNumber DESC;
)",
                                     Program::sql().select));

        query.bindValue(":itemId", itemId.toUrl());

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
        } else {
            std::vector<Program> programes;
            while (query.next()) {
                auto& s = programes.emplace_back();
                int   i = 0;
                load_query(query, s, i);
            }
            co_return programes;
        }
        co_return std::nullopt;
    }

    void reload() override {
        set_status(Status::Querying);
        auto self   = helper::QWatcher { this };
        auto itemId = m_radio_id;
        spawn( [self, itemId] -> task<void> {
            auto sql        = App::instance()->album_sql();
            bool needReload = false;

            bool                                synced { 0 };
            std::optional<Radio>                radio;
            std::optional<std::vector<Program>> programes;
            for (;;) {
                radio     = co_await self->query_radio(itemId);
                programes = co_await self->query_programs(itemId);
                if (! synced &&
                    (! radio || ! programes || radio->programCount != (int)programes->size())) {
                    co_await SyncAPi::sync_item(itemId);
                    synced = true;
                    continue;
                }
                break;
            }

            co_await asio::post(
                asio::bind_executor(Global::instance()->qexecutor(), asio::use_awaitable));
            if (self) {
                self->tdata()->setInfo(radio);
                self->tdata()->resetModel(programes);
                self->set_status(Status::Finished);
            }
            co_return;
        });
    }

    Q_SLOT void reset() {}

private:
    model::ItemId m_radio_id;
};

} // namespace qcm::query
