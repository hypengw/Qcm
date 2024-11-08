#pragma once

#pragma once

#include <QQmlEngine>

#include "Qcm/query/query.h"
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

class DjradioDetail : public meta_model::QGadgetListModel<Program> {
    Q_OBJECT

    Q_PROPERTY(Djradio info READ info NOTIFY infoChanged)

    using base_type = meta_model::QGadgetListModel<Program>;

public:
    DjradioDetail(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<Program>(parent), m_has_more(true) {}

    auto info() const -> const Djradio& { return m_info; }
    void setInfo(const std::optional<Djradio>& v) {
        m_info = v.value_or(Djradio {});
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
    bool    m_has_more;
    Djradio m_info;
};

class DjradioDetailQuery : public Query<DjradioDetail> {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)
    Q_PROPERTY(DjradioDetail* data READ tdata NOTIFY itemIdChanged FINAL)
public:
    DjradioDetailQuery(QObject* parent = nullptr): Query<DjradioDetail>(parent) {
        connect(this, &DjradioDetailQuery::itemIdChanged, this, &DjradioDetailQuery::reload);
    }

    auto itemId() const -> const model::ItemId& { return m_djradio_id; }
    void setItemId(const model::ItemId& v) {
        if (ycore::cmp_exchange(m_djradio_id, v)) {
            itemIdChanged();
        }
    }

    Q_SIGNAL void itemIdChanged();

public:
    auto query_djradio(model::ItemId itemId) -> task<std::optional<Djradio>> {
        auto sql = App::instance()->album_sql();
        co_await asio::post(asio::bind_executor(sql->get_executor(), use_task));

        auto query = sql->con()->query();
        query.prepare_sv(std::format(R"(
SELECT 
    {}
FROM djradio
WHERE djradio.itemId = :itemId
GROUP BY djradio.itemId;
)",
                                     model::Radio::sql().select));
        query.bindValue(":itemId", itemId.toUrl());

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
        } else if (query.next()) {
            Djradio dj;
            int     i = 0;
            load_query(dj, query, i);
            co_return dj;
        }
        co_return std::nullopt;
    }

    auto query_programs(model::ItemId itemId) -> task<std::optional<std::vector<Program>>> {
        auto sql = App::instance()->album_sql();
        co_await asio::post(asio::bind_executor(sql->get_executor(), use_task));
        auto query = sql->con()->query();
        query.prepare(uR"(
SELECT 
    %1
FROM program
WHERE program.radioId = :itemId
GROUP BY program.itemId
ORDER BY program.serialNumber DESC;
)"_s.arg(Program::Select));

        query.bindValue(":itemId", itemId.toUrl());

        if (! query.exec()) {
            ERROR_LOG("{}", query.lastError().text());
        } else {
            std::vector<Program> programes;
            while (query.next()) {
                auto& s = programes.emplace_back();
                int   i = 0;
                load_query(s, query, i);
            }
            co_return programes;
        }
        co_return std::nullopt;
    }

    void reload() override {
        set_status(Status::Querying);
        auto ex     = asio::make_strand(pool_executor());
        auto self   = helper::QWatcher { this };
        auto itemId = m_djradio_id;
        spawn(ex, [self, itemId] -> task<void> {
            auto sql        = App::instance()->album_sql();
            bool needReload = false;

            bool                                synced { 0 };
            std::optional<Djradio>              djradio;
            std::optional<std::vector<Program>> programes;
            for (;;) {
                djradio   = co_await self->query_djradio(itemId);
                programes = co_await self->query_programs(itemId);
                if (! synced &&
                    (! djradio || ! programes || djradio->programCount != (int)programes->size())) {
                    co_await SyncAPi::sync_item(itemId);
                    synced = true;
                    continue;
                }
                break;
            }

            co_await asio::post(
                asio::bind_executor(Global::instance()->qexecutor(), asio::use_awaitable));
            if (self) {
                self->tdata()->setInfo(djradio);
                self->tdata()->resetModel(programes);
                self->set_status(Status::Finished);
            }
            co_return;
        });
    }

    Q_SLOT void reset() {}

private:
    model::ItemId m_djradio_id;
};

} // namespace qcm::query
