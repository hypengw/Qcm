#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>

#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/user_cloud.h"

#include "meta_model/qgadgetlistmodel.h"

#include "core/log.h"

namespace qcm
{
namespace model
{
struct UserCloudItem {
    Q_GADGET
public:
    GADGET_PROPERTY_DEF(SongId, itemId, id)
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, picUrl, picUrl)
    GADGET_PROPERTY_DEF(QString, albumName, albumName)
    GADGET_PROPERTY_DEF(QString, artistName, artistName)
    GADGET_PROPERTY_DEF(qint32, bitrate, bitrate)
    GADGET_PROPERTY_DEF(QDateTime, addTime, addTime)
    GADGET_PROPERTY_DEF(Song, song, song)

    std::strong_ordering operator<=>(const UserCloudItem&) const = default;
};
} // namespace model
} // namespace qcm

DEFINE_CONVERT(qcm::model::UserCloudItem, ncm::model::UserCloudItem) {
    convert(out.id, in.songId);
    convert(out.name, in.songName);
    convert(out.albumName, in.album);
    convert(out.artistName, in.artist);
    convert(out.bitrate, in.bitrate);
    convert(out.addTime, in.addTime);
    convert(out.song, in.simpleSong);
}

namespace qcm
{
namespace model
{

class UserCloud : public meta_model::QGadgetListModel<UserCloudItem> {
    Q_OBJECT
public:
    UserCloud(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<UserCloudItem>(parent), m_has_more(true) {}
    using out_type = ncm::api_model::UserCloud;

    void handle_output(const out_type& re, const auto& input) {
        if (input.offset == 0) {
            auto in_ = convert_from<std::vector<UserCloudItem>>(re.data);
            convertModel(in_, [](const UserCloudItem& it) {
                return convert_from<std::string>(it.id);
            });
        } else if (input.offset == (int)rowCount()) {
            insert(rowCount(), convert_from<std::vector<UserCloudItem>>(re.data));
        }
        m_has_more = re.hasMore;
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
static_assert(modelable<UserCloud, ncm::api::UserCloud>);
} // namespace model

using UserCloudQuerier_base = ApiQuerier<ncm::api::UserCloud, model::UserCloud>;
class UserCloudQuerier : public UserCloudQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    UserCloudQuerier(QObject* parent = nullptr): UserCloudQuerier_base(parent) {}

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
