#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>

#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/artist_sublist.h"
#include "meta_model/qgadgetlistmodel.h"

#include "core/log.h"

namespace qcm
{
namespace model
{
struct ArtistSublistItem {
    Q_GADGET
public:
    GATGET_PROPERTY(ArtistId, itemId, itemId)
    GATGET_PROPERTY(QString, name, name)
    GATGET_PROPERTY(QString, picUrl, picUrl)
    GATGET_PROPERTY(qint32, albumSize, albumSize)
};
} // namespace model
} // namespace qcm

template<>
struct To<qcm::model::ArtistSublistItem> {
    static auto from(const ncm::model::ArtistSublistItem& in) {
        qcm::model::ArtistSublistItem o;
        CONVERT_PROPERTY(o.itemId, in.id);
        CONVERT_PROPERTY(o.name, in.name);
        CONVERT_PROPERTY(o.picUrl, in.picUrl);
        CONVERT_PROPERTY(o.albumSize, in.albumSize);
        return o;
    };
};

namespace qcm
{
namespace model
{

class ArtistSublist : public meta_model::QGadgetListModel<ArtistSublistItem> {
    Q_OBJECT
public:
    ArtistSublist(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<ArtistSublistItem>(parent), m_has_more(true) {}
    using out_type = ncm::api_model::ArtistSublist;

    void handle_output(const out_type& re, const auto& input) {
        if (input.offset != (int)rowCount()) {
            return;
        }
        if (! re.data.empty()) {
            auto in_ = To<std::vector<ArtistSublistItem>>::from(re.data);
            for (auto& el : in_) {
                insert(rowCount(), el);
            }
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
    std::vector<ArtistSublistItem> m_items;
    bool                           m_has_more;
};
static_assert(modelable<ArtistSublist, ncm::api::ArtistSublist>);
} // namespace model

using ArtistSublistQuerier_base = ApiQuerier<ncm::api::ArtistSublist, model::ArtistSublist>;
class ArtistSublistQuerier : public ArtistSublistQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    ArtistSublistQuerier(QObject* parent = nullptr): ArtistSublistQuerier_base(parent) {}

    FORWARD_PROPERTY(qint32, offset, offset)
    FORWARD_PROPERTY(qint32, limit, limit)

public:
    void fetch_more(qint32 cur_count) override { set_offset(cur_count); }
};

} // namespace qcm
