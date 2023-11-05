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
struct ArtistSublistItem : public Artist {
    Q_GADGET
public:
    std::strong_ordering operator<=>(const ArtistSublistItem&) const = default;
};
} // namespace model
} // namespace qcm

DEFINE_CONVERT(qcm::model::ArtistSublistItem, ncm::model::ArtistSublistItem) {
    convert(out.id, in.id);
    convert(out.name, in.name);
    convert(out.picUrl, in.picUrl);
    convert(out.albumSize, in.albumSize);
    out.followed = true;
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
        auto in_ = convert_from<std::vector<ArtistSublistItem>>(re.data);
        if (input.offset == 0) {
            convertModel(in_, [](const ArtistSublistItem& it) -> std::string {
                return convert_from<std::string>(it.id);
            });
        } else if (input.offset == (int)rowCount()) {
            insert(rowCount(), in_);
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

public slots:
    void reset() {
        api().input.offset = 0;
        reload();
    }
};

} // namespace qcm
