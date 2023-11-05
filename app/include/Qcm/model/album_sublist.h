#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>
#include <set>

#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/album_sublist.h"

#include "meta_model/qgadgetlistmodel.h"

#include "core/log.h"
#include "core/qlist_helper.h"

namespace qcm
{
namespace model
{
struct AlbumSublistItem : public Album {
    Q_GADGET
public:
    GADGET_PROPERTY_DEF(QDateTime, subTime, subTime)
    std::strong_ordering operator<=>(const AlbumSublistItem&) const = default;
};
} // namespace model
} // namespace qcm

DEFINE_CONVERT(qcm::model::AlbumSublistItem, ncm::model::AlbumSublistItem) {
    convert(out.id, in.id);
    convert(out.name, in.name);
    convert(out.artists, in.artists);
    convert(out.picUrl, in.picUrl);
    convert(out.subTime, in.subTime);
    convert(out.trackCount, in.size);
    out.subscribed = true;
}
namespace qcm
{
namespace model
{

class AlbumSublist : public meta_model::QGadgetListModel<AlbumSublistItem> {
    Q_OBJECT
public:
    AlbumSublist(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<AlbumSublistItem>(parent), m_has_more(true) {}
    using out_type = ncm::api_model::AlbumSublist;

    void handle_output(const out_type& re, const auto& input) {
        if (input.offset == 0) {
            auto in_ = convert_from<std::vector<AlbumSublistItem>>(re.data);
            convertModel(in_, [](const AlbumSublistItem& it) -> std::string {
                return convert_from<std::string>(it.id);
            });
        } else if (input.offset == (int)rowCount()) {
            insert(rowCount(), convert_from<std::vector<AlbumSublistItem>>(re.data));
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
static_assert(modelable<AlbumSublist, ncm::api::AlbumSublist>);
} // namespace model

using AlbumSublistQuerier_base = ApiQuerier<ncm::api::AlbumSublist, model::AlbumSublist>;
class AlbumSublistQuerier : public AlbumSublistQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    AlbumSublistQuerier(QObject* parent = nullptr): AlbumSublistQuerier_base(parent) {}

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
