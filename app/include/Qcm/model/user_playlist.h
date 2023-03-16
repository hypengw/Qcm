#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>

#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/user_playlist.h"

#include "core/log.h"

namespace qcm
{
namespace model
{
struct UserPlaylistItem {
public:
    PlaylistId id;
    QString    name;
    QString    picUrl;
    qint32     playCount;
    qint32     trackCount;
};
} // namespace model
} // namespace qcm

template<>
struct To<qcm::model::UserPlaylistItem> {
    static auto from(const ncm::model::UserPlaylistItem& in) {
        qcm::model::UserPlaylistItem o;
        CONVERT_PROPERTY(o.id, in.id);
        CONVERT_PROPERTY(o.name, in.name);
        CONVERT_PROPERTY(o.picUrl, in.coverImgUrl);
        CONVERT_PROPERTY(o.playCount, in.playCount);
        CONVERT_PROPERTY(o.trackCount, in.trackCount);
        return o;
    };
};

namespace qcm
{
namespace model
{

class UserPlaylist : public QAbstractListModel {
    Q_OBJECT
public:
    UserPlaylist(QObject* parent = nullptr): QAbstractListModel(parent), m_has_more(true) {}
    using out_type = ncm::api_model::UserPlaylist;

    void handle_output(const out_type& re, const auto& input) {
        if (input.offset != (int)rowCount()) {
            return;
        }
        if (! re.playlist.empty()) {
            auto in_      = To<std::vector<UserPlaylistItem>>::from(re.playlist);
            auto in_count = in_.size();
            auto count    = rowCount();
            beginInsertRows({}, count, count + in_count - 1);
            m_items.insert(m_items.end(), in_.begin(), in_.end());
            endInsertRows();
        }
        m_has_more = re.more;
    }

    enum Role
    {
        IdRole,
        NameRole,
        PicRole,
        TrackCountRole,
    };
    Q_ENUM(Role)

    // list model override
    int      rowCount(const QModelIndex& = QModelIndex()) const override { return m_items.size(); }
    QVariant data(const QModelIndex& index, int role = NameRole) const override {
        auto  row = index.row();
        auto& d   = m_items.at(row);
        switch (role) {
        case IdRole: return QVariant::fromValue(d.id);
        case PicRole: return d.picUrl;
        case TrackCountRole: return d.trackCount;
        case NameRole:
        default: return d.name;
        }
    };
    QHash<int, QByteArray> roleNames() const override {
        return { { IdRole, "itemId" },
                 { NameRole, "name" },
                 { PicRole, "picUrl" },
                 { TrackCountRole, "trackCount" } };
    }
    bool canFetchMore(const QModelIndex&) const override { return m_has_more; }
    void fetchMore(const QModelIndex&) override {
        m_has_more = false;
        emit fetchMoreReq(rowCount());
    }
signals:
    void fetchMoreReq(qint32);

private:
    std::vector<UserPlaylistItem> m_items;
    bool                          m_has_more;
};
static_assert(modelable<UserPlaylist, ncm::api::UserPlaylist>);
} // namespace model

using UserPlaylistQuerier_base = ApiQuerier<ncm::api::UserPlaylist, model::UserPlaylist>;
class UserPlaylistQuerier : public UserPlaylistQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    UserPlaylistQuerier(QObject* parent = nullptr): UserPlaylistQuerier_base(parent) {}

    FORWARD_PROPERTY(model::UserId, uid, uid)
    FORWARD_PROPERTY(qint32, offset, offset)
    FORWARD_PROPERTY(qint32, limit, limit)

public:
    void fetch_more(qint32 cur_count) override { set_offset(cur_count); }
};

} // namespace qcm
