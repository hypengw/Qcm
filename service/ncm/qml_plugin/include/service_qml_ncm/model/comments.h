#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>

#include "service_qml_ncm/api.h"
#include "service_qml_ncm/model.h"
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
        using CT = ncm::params::Comments::Type;
        using IT = ncm::model::IdType;
        if (v != m_id) {
            auto& id   = this->api().input.id;
            auto& type = this->api().input.type;
            m_id       = v;
            if (v.canConvert<model::ItemId>()) {
                auto item_id = v.value<model::ItemId>();
                id.id = item_id.id().toStdString();
                switch (ncm::ncm_id_type(item_id)) {
                case IT::Album: {
                    type = CT::Album;
                    break;
                }
                case IT::Song: {
                    type = CT::Song;
                    break;
                }
                case IT::Playlist: {
                    type = CT::Playlist;
                    break;
                }
                case IT::Program: {
                    type = CT::Program;
                    break;
                }
                default: {
                    _assert_msg_rel_(false, "unsupport comment type: {}", item_id.type());
                }
                }
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
