#pragma once

#include <QQmlEngine>
#include <QAbstractListModel>

#include "core/asio/basic.h"
#include "qcm_interface/query.h"
#include "Qcm/model/comment.hpp"
#include "meta_model/qgadget_list_model.hpp"
#include "core/log.h"
#include "Qcm/app.hpp"

namespace qcm
{

class Comments : public meta_model::QGadgetListModel<model::Comment> {
    Q_OBJECT

    Q_PROPERTY(qint32 total READ total NOTIFY totalChanged)
public:
    Comments(QObject* parent = nullptr)
        : meta_model::QGadgetListModel<model::Comment>(parent), m_has_more(true), m_total(0) {}

    // void handle_output(const out_type& re, const auto& input) {
    //     if (input.offset != (int)rowCount()) {
    //         return;
    //     }
    //     insert(rowCount(), convert_from<std::vector<Comment>>(re.comments));

    //    if (auto new_total = std::max(re.total, (i64)rowCount()); m_total != new_total) {
    //        m_total = new_total;
    //        Q_EMIT totalChanged();
    //    }

    //    m_has_more = re.more;
    //}

    bool canFetchMore(const QModelIndex&) const override { return m_has_more; }
    void fetchMore(const QModelIndex&) override {
        m_has_more = false;
        emit fetchMoreReq(rowCount());
    }
    void setHasMore(bool v) { m_has_more = v; }
    void setTotal(i32 v) {
        if (ycore::cmp_exchange(m_total, v)) {
            totalChanged();
        }
    }

    qint32 total() const { return m_total; }

    Q_SIGNAL void fetchMoreReq(qint32);
    Q_SIGNAL void totalChanged();

private:
    bool m_has_more;
    i64  m_total;
};

class CommentsQuery : public QueryList, public QueryExtra<Comments> {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qcm::model::ItemId itemId READ itemId WRITE setItemId NOTIFY itemIdChanged)

public:
    CommentsQuery(QObject* parent = nullptr): , public QueryExtra<Comments>(parent) {
        connect_requet_reload(&CommentsitemIdChanged);
        connect(tdata(), &Comments::fetchMoreReq, this, &CommentssetOffset);
    }

public:
    auto itemId() const { return m_id; }
    void setItemId(const model::ItemId v) {
        if (ycore::cmp_exchange(m_id, v)) {
            itemIdChanged();
        }
    }
    Q_SIGNAL void itemIdChanged();

    void reload() override {
        set_status(Status::Querying);
        auto self   = helper::QWatcher { this };
        auto itemId = m_id;
        auto offset = this->offset();
        auto limit  = this->limit();
        auto client = Global::instance()->qsession()->client();

        if (! client || ! itemId.valid()) {
            return;
        }

        spawn([self, itemId, offset, limit, client = client.value()] -> task<void> {
            auto                     sql = App::instance()->item_sql();
            std::vector<model::Song> items;
            i32                      total { 0 };
            auto out = co_await client.api->comments(client, itemId, offset, limit, total);
            co_await asio::post(
                asio::bind_executor(qcm::qexecutor(), asio::use_awaitable));
            if (self) {
                if (out) {
                    self->tdata()->setHasMore(out->size());
                    self->tdata()->insert(self->tdata()->rowCount(), out->to_ref_view());
                    if (offset == 0) {
                        self->tdata()->setTotal(total);
                    }
                    self->set_status(Status::Finished);
                } else {
                    self->set_error(convert_from<QString>(out.error().what()));
                    self->set_status(Status::Error);
                }
            }
            co_return;
        });
    }

private:
    model::ItemId m_id;
};

} // namespace qcm
