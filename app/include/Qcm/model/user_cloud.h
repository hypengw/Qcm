#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>

#include "Qcm/api.h"
#include "Qcm/model.h"
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
    GATGET_PROPERTY(SongId, itemId, id)
    GATGET_PROPERTY(QString, name, name)
    GATGET_PROPERTY(QString, picUrl, picUrl)
    GATGET_PROPERTY(QString, albumName, albumName)
    GATGET_PROPERTY(QString, artistName, artistName)
    GATGET_PROPERTY(qint32, bitrate, bitrate)
    GATGET_PROPERTY(QDateTime, addTime, addTime)
    GATGET_PROPERTY(Song, song, song)

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
