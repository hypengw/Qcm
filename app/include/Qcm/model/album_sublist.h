#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>
#include <set>

#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/album_sublist.h"

#include "meta_model/qgadgetlistmodel.h"

#include "core/log.h"

namespace qcm
{
namespace model
{
struct AlbumSublistItem {
    Q_GADGET
public:
    GATGET_PROPERTY(AlbumId, itemId, id)
    GATGET_PROPERTY(QString, name, name)
    GATGET_PROPERTY(QString, picUrl, picUrl)
    GATGET_PROPERTY(std::vector<Artist>, artists, artists)

    std::strong_ordering operator<=>(const AlbumSublistItem&) const = default;
};
} // namespace model
} // namespace qcm

template<>
struct To<qcm::model::AlbumSublistItem> {
    static auto from(const ncm::model::AlbumSublistItem& in) {
        qcm::model::AlbumSublistItem o;
        convert(o.id, in.id);
        convert(o.name, in.name);
        convert(o.artists, in.artists);
        convert(o.picUrl, in.picUrl);
        return o;
    };
};

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
            auto in_ = To<std::vector<AlbumSublistItem>>::from(re.data);
            convertModel(in_, [](const AlbumSublistItem& it) -> std::string {
                return To<std::string>::from(it.id);
            });
            m_has_more = re.hasMore;
        } else if (input.offset == (int)rowCount()) {
            if (! re.data.empty()) {
                auto in_ = To<std::vector<AlbumSublistItem>>::from(re.data);
                for (auto& el : in_) {
                    insert(rowCount(), el);
                }
            }
            m_has_more = re.hasMore;
        }
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
