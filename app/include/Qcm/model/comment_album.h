#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>

#include "Qcm/api.h"
#include "Qcm/model.h"
#include "ncm/api/comment_album.h"
#include "meta_model/qgadgetlistmodel.h"

#include "core/log.h"

namespace qcm
{
namespace model
{

class CommentAlbum : public meta_model::QGadgetListModel<Comment> {
    Q_OBJECT

    Q_PROPERTY(qint32 total READ total NOTIFY totalChanged)
public:
    CommentAlbum(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<Comment>(parent), m_has_more(true), m_total(0) {}
    using out_type = ncm::api_model::CommentAlbum;

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
static_assert(modelable<CommentAlbum, ncm::api::CommentAlbum>);
} // namespace model

using CommentAlbumQuerier_base = ApiQuerier<ncm::api::CommentAlbum, model::CommentAlbum>;
class CommentAlbumQuerier : public CommentAlbumQuerier_base {
    Q_OBJECT
    QML_ELEMENT
public:
    CommentAlbumQuerier(QObject* parent = nullptr): CommentAlbumQuerier_base(parent) {}

    FORWARD_PROPERTY(model::AlbumId, itemId, id)
    FORWARD_PROPERTY(qint32, offset, offset)
    FORWARD_PROPERTY(qint32, limit, limit)

public:
    void fetch_more(qint32 cur_count) override { set_offset(cur_count); }
};

} // namespace qcm
