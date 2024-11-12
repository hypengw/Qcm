#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>

#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
#include "ncm/api/user_cloud.h"
#include "qcm_interface/oper/song_oper.h"

#include "meta_model/qgadgetlistmodel.h"
#include "qcm_interface/model/query_model.h"

#include "core/log.h"

DEFINE_CONVERT(qcm::query::Song, ncm::model::UserCloudItem) {
    auto oper = qcm::oper::SongOper(out);
    convert(oper, in.simpleSong);
    convert(out.album.name, in.album);
    convert(out.album.picUrl, in.simpleSong.al.picUrl.value_or(""s));

    oper.set_id(convert_from<qcm::model::ItemId>(in.songId));
    for (usize i = 0; i < in.simpleSong.ar.size(); i++) {
        auto& a    = out.artists.emplace_back();
        auto  oper = qcm::oper::ArtistReferOper(a);
        convert(oper, in.simpleSong.ar[i]);
    }
    if (out.artists.size() == 1 && out.artists.front().name.isEmpty()) {
        convert(out.artists.front().name, in.artist);
    }
    if (in.simpleSong.dt.milliseconds == 0 && in.bitrate) {
        auto t = in.simpleSong.dt;
        // seems netease use 1024
        t.milliseconds = (in.fileSize * 8.0 / 1024.0) / in.bitrate * 1000.0;
        convert(out.duration, t);
    }
    // convert(out.addTime, in.addTime);
}

namespace qcm
{
namespace model
{

class UserCloud : public meta_model::QGadgetListModel<query::Song> {
    Q_OBJECT
public:
    UserCloud(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<query::Song>(parent), m_has_more(true) {}
    using out_type = ncm::api_model::UserCloud;

    void handle_output(const out_type& re, const auto& input) {
        if (input.offset == 0) {
            auto in_ = convert_from<std::vector<query::Song>>(re.data);
            convertModel(in_, [](const auto& it) {
                return convert_from<std::string>(it.id.toUrl().toString());
            });
        } else if (input.offset == (int)rowCount()) {
            insert(rowCount(), convert_from<std::vector<query::Song>>(re.data));
        }
        m_has_more = re.hasMore;
    }

    bool canFetchMore(const QModelIndex&) const override { return m_has_more; }
    void fetchMore(const QModelIndex&) override {
        m_has_more = false;
        emit fetchMoreReq(rowCount());
    }
    Q_SIGNAL void fetchMoreReq(qint32);

    Q_INVOKABLE query::Song itemAt(qint32 idx) const {
        if (idx < rowCount() && idx >= 0)
            return at(idx);
        else
            return {};
    }
    Q_INVOKABLE QVariantList songs() const {
        auto view =
            std::views::transform(std::views::filter(std::ranges::subrange(begin(), end(), size()),
                                                     [](const auto& el) {
                                                         return el.canPlay;
                                                     }),
                                  [](const auto& el) {
                                      return QVariant::fromValue(el);
                                  });
        return QVariantList { view.begin(), view.end() };
    }

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
    void        fetch_more(qint32 cur_count) override { set_offset(cur_count); }
    Q_SLOT void reset() {
        api().input.offset = 0;
        reload();
    }
};

} // namespace qcm
