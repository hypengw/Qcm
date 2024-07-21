#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>

#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/user_playlist.h"

#include "meta_model/qgadgetlistmodel.h"

#include "core/log.h"

namespace qcm
{
namespace model
{

class UserPlaylist : public meta_model::QGadgetListModel<Playlist> {
    Q_OBJECT
    Q_PROPERTY(
        model::ItemId onlyUserId READ onlyUserId WRITE setOnlyUserId NOTIFY onlyUserIdChanged)
public:
    UserPlaylist(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<Playlist>(parent), m_has_more(true) {}
    using out_type = ncm::api_model::UserPlaylist;

    model::ItemId onlyUserId() const { return m_user_id; }
    void          setOnlyUserId(const model::ItemId& v) {
        if (std::exchange(m_user_id, v) != v) {
            Q_EMIT onlyUserIdChanged();
            m_has_more = true;
            resetModel();
        }
    }

    void handle_output(const out_type& re, const auto& input) {
        auto in_ = convert_from<std::vector<Playlist>>(re.playlist);

        m_has_more = re.more;
        if (m_user_id.valid()) {
            if (std::erase_if(in_, [this](const Playlist& pl) -> bool {
                    return pl.userId != m_user_id;
                })) {
                m_has_more = false;
            }
        }

        if (input.offset == 0) {
            convertModel(in_, [](const Playlist& it) {
                return convert_from<std::string>(it.id);
            });
        } else if (input.offset == (int)rowCount()) {
            insert(rowCount(), in_);
        }
    }

    bool canFetchMore(const QModelIndex&) const override { return m_has_more; }
    void fetchMore(const QModelIndex&) override {
        m_has_more = false;
        emit fetchMoreReq(rowCount());
    }
signals:
    void fetchMoreReq(qint32);
    void onlyUserIdChanged();

private:
    bool          m_has_more;
    model::ItemId m_user_id;
};
static_assert(modelable<UserPlaylist, ncm::api::UserPlaylist>);
} // namespace model

using UserPlaylistQuerier_base = ApiQuerier<ncm::api::UserPlaylist, model::UserPlaylist>;
class UserPlaylistQuerier : public UserPlaylistQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    UserPlaylistQuerier(QObject* parent = nullptr): UserPlaylistQuerier_base(parent) {}

    FORWARD_PROPERTY(model::ItemId, uid, uid)
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
