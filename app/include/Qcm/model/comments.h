#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>

#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/comments.h"
#include "meta_model/qgadgetlistmodel.h"

#include "core/log.h"

namespace qcm
{
namespace model
{

class Comments : public meta_model::QGadgetListModel<Comment> {
    Q_OBJECT

    Q_PROPERTY(qint32 total READ total NOTIFY totalChanged)
public:
    Comments(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<Comment>(parent), m_has_more(true), m_total(0) {}
    using out_type = ncm::api_model::Comments;

    void handle_output(const out_type& re, const auto& input) {
        if (input.offset != (int)rowCount()) {
            return;
        }
        insert(rowCount(), convert_from<std::vector<Comment>>(re.comments));

        if (auto new_total = std::max(re.total, (i64)rowCount()); m_total != new_total) {
            m_total = new_total;
            Q_EMIT totalChanged();
        }

        m_has_more = re.more;
    }

    bool canFetchMore(const QModelIndex&) const override { return m_has_more; }
    void fetchMore(const QModelIndex&) override {
        m_has_more = false;
        emit fetchMoreReq(rowCount());
    }

    qint32 total() const { return m_total; }
signals:
    void fetchMoreReq(qint32);
    void totalChanged();

private:
    bool m_has_more;
    i64  m_total;
};
static_assert(modelable<Comments, ncm::api::Comments>);
} // namespace model

using CommentsQuerier_base = ApiQuerier<ncm::api::Comments, model::Comments>;
class CommentsQuerier : public CommentsQuerier_base {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariant itemId READ itemId WRITE set_itemId NOTIFY changed_itemId)

public:
    CommentsQuerier(QObject* parent = nullptr)
        : CommentsQuerier_base(parent), m_id(QVariant::fromValue(model::ItemId())) {}

    void fetch_more(qint32 cur_count) override { set_offset(cur_count); }

    FORWARD_PROPERTY(qint32, offset, offset)
    FORWARD_PROPERTY(qint32, limit, limit)

public:
    QVariant itemId() const { return convert_from<QVariant>(m_id); }
    void     set_itemId(QVariant v) {
        if (v != m_id) {
            auto& id   = this->api().input.id;
            auto& type = this->api().input.type;
            m_id       = v;
            if (v.canConvert<model::AlbumId>()) {
                convert(id, v.value<model::AlbumId>());
                type = ncm::model::IDType::Album;
            } else if (v.canConvert<model::PlaylistId>()) {
                convert(id, v.value<model::PlaylistId>());
                type = ncm::model::IDType::Playlist;
            } else if (v.canConvert<model::SongId>()) {
                convert(id, v.value<model::SongId>());
                type = ncm::model::IDType::Song;
            } else if(v.canConvert<model::ProgramId>()) {
                convert(id, v.value<model::ProgramId>());
                type = ncm::model::IDType::Program;
            }
            this->mark_dirty();
            emit changed_itemId();
            this->reload_if_needed();
        }
    }

Q_SIGNALS:
    void changed_itemId();

private:
    QVariant m_id;
};

} // namespace qcm
