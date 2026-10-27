module;
#include "Qcm/query/comments_query.moc.h"
module qcm;
import :query.comments;
import :app;

namespace qcm
{
void CommentView::applyReaction() {
    if (m_reaction.liked)
        m_value.setLiked(*m_reaction.liked);
    else
        m_value.clearLiked();
    if (m_reaction.count)
        m_value.setLikeCount(*m_reaction.count);
    else
        m_value.clearLikeCount();
}
CommentStore::CommentStore(QObject* parent): QObject(parent) {
    auto app = App::instance();
    connect(app->backend(), &Backend::disconnected, this, &CommentStore::clear);
    auto providers = app->provider_status();
    connect(providers, &QAbstractItemModel::dataChanged, this, [this] {
        reconcileProvider();
    });
    connect(providers, &QAbstractItemModel::modelReset, this, [this] {
        reconcileProvider();
    });
    connect(providers, &QAbstractItemModel::rowsRemoved, this, [this] {
        reconcileProvider();
    });
    connect(providers, &ProviderStatusModel::activedIdsChanged, this, [this] {
        reconcileProvider();
    });
}
CommentStore::~CommentStore() = default;
void CommentStore::reconcileProvider() {
    if (! m_provider_id) return;
    auto providers = App::instance()->provider_status();
    auto value     = providers->itemById({ enums::ItemType::ItemProvider, m_provider_id })
                         .value<model::ProviderStatus>();
    if (! providers->actived(m_provider_id) || value.epoch() != m_epoch) clear();
}
CommentView* CommentStore::view(const QString& id) {
    if (id.isEmpty()) return nullptr;
    auto& value = m_entries[id];
    if (! value) value = new CommentView(this);
    return value;
}
void CommentStore::clear() {
    ++m_generation;
    m_jobs.cancel();
    m_scope.clear();
    m_item_id     = model::ItemId{};
    m_provider_id = 0;
    m_can_like    = false;
    for (auto entry : std::as_const(m_entries)) entry->deleteLater();
    m_entries.clear();
    Q_EMIT changed();
    Q_EMIT invalidated();
}
void CommentStore::bind(model::ItemId id, const msg::model::CommentsResult& result) {
    if (! m_scope.isEmpty() && (m_scope != result.scope() || id != m_item_id)) clear();
    m_item_id     = id;
    m_scope       = result.scope();
    m_provider_id = result.providerId();
    m_epoch       = result.epoch();
    m_can_like    = result.page().canLike();
    Q_EMIT changed();
}
void CommentStore::merge(const QList<msg::model::Comment>& values, quint64 revision) {
    for (const auto& value : values) {
        auto entry = view(value.id_proto());
        if (! entry) continue;
        entry->m_value = value;
        entry->m_reaction.read(
            value.hasLiked() ? std::optional<bool>(value.liked()) : std::nullopt,
            value.hasLikeCount() ? std::optional<std::uint64_t>(value.likeCount()) : std::nullopt,
            revision);
        if (! entry->uncertain()) entry->m_error.clear();
        entry->applyReaction();
        Q_EMIT entry->changed();
        merge(value.previews(), revision);
    }
}
void CommentStore::setLiked(const QString& id, bool value) {
    auto entry = m_entries.value(id);
    if (m_closed || ! canLike() || ! entry || entry->m_value.deleted() ||
        entry->m_reaction.liked == value || ! entry->m_reaction.begin(tick()))
        return;
    entry->m_error.clear();
    Q_EMIT entry->changed();
    msg::SetCommentLikeReq req;
    req.setId_proto(m_item_id.id());
    req.setItemType(rstd::into(m_item_id.type()));
    req.setScope(m_scope);
    req.setCommentId(id);
    req.setValue(value);
    auto self       = QWatcher { this };
    auto backend    = App::instance()->backend();
    auto generation = m_generation;
    m_jobs.spawn([self, backend, req, generation, id]() mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor() || ! self || generation != self->m_generation)
            co_return;
        auto entry = self->m_entries.value(id);
        if (! entry) co_return;
        if (! rsp || ! rsp->hasResult()) {
            entry->m_reaction.fail(self->tick());
            entry->m_error = rsp ? tr("Invalid reaction response")
                                 : QString::fromStdString(rsp.unwrap_err_unchecked().message);
            Q_EMIT entry->changed();
            Q_EMIT self->reactionFailed(entry->m_error);
            Q_EMIT self->refreshRequested();
            co_return;
        }
        auto result = rsp->result();
        entry->m_reaction.complete(
            result.liked(),
            result.hasLikeCount() ? std::optional<std::uint64_t>(result.likeCount()) : std::nullopt,
            self->tick());
        entry->applyReaction();
        Q_EMIT entry->changed();
    });
}
QVariant CommentListModel::data(const QModelIndex& index, int role) const {
    if (! m_store || ! index.isValid() || index.row() < 0 || index.row() >= count() ||
        role != Qt::UserRole)
        return {};
    return QVariant::fromValue(m_store->view(m_ids[index.row()]));
}
void CommentListModel::reset(CommentStore* store) {
    beginResetModel();
    m_store = store;
    m_ids.clear();
    m_seen.clear();
    endResetModel();
    Q_EMIT countChanged();
}
void CommentListModel::append(const QList<msg::model::Comment>& values) {
    QStringList added;
    for (const auto& value : values)
        if (! m_seen.contains(value.id_proto())) {
            added.append(value.id_proto());
            m_seen.insert(value.id_proto());
        }
    if (added.isEmpty()) return;
    beginInsertRows({}, count(), count() + static_cast<int>(added.size()) - 1);
    m_ids.append(added);
    endInsertRows();
    Q_EMIT countChanged();
}
CommentsQuery::CommentsQuery(QObject* parent)
    : Query(parent), m_owned_store(new CommentStore(this)), m_model(this) {
    setForwardError(false);
    setStore(m_owned_store);
    connect(App::instance()->backend(), &Backend::connected, this, [this] {
        if (! m_stopped) delayReload();
    });
}
void CommentsQuery::setStore(CommentStore* store) {
    if (m_store == store) return;
    if (m_store) disconnect(m_store, nullptr, this, nullptr);
    invalidateRequests();
    m_store = store;
    if (store) {
        connect(store, &CommentStore::invalidated, this, [this] {
            reset();
            if (!m_stopped && m_store && !m_store->closed()) {
                setError(tr("Comments context changed"));
                setStatus(Status::Error);
            }
        });
        connect(store, &CommentStore::refreshRequested, this, &CommentsQuery::delayReload);
    }
    reset();
    Q_EMIT storeChanged();
}
void CommentsQuery::reset() {
    invalidateRequests();
    m_model.reset(m_store);
    m_token.clear();
    m_total.clear();
    m_orders.clear();
    m_supported = m_loaded = m_has_more = m_loading_more = m_can_reply = false;
    setError({});
    setStatus(Status::Uninitialized);
    Q_EMIT changed();
}
void CommentsQuery::setItemId(model::ItemId id) {
    if (m_item_id == id) return;
    m_item_id = id;
    if (m_store && ! m_store->matchesTarget(id)) m_store->clear();
    reset();
    Q_EMIT itemIdChanged();
}
void CommentsQuery::setParentId(const QString& id) {
    if (m_parent_id == id) return;
    m_parent_id = id;
    reset();
    Q_EMIT parentIdChanged();
}
void CommentsQuery::setOrder(int order) {
    if ((order != 0 && order != 1) || m_order == order) return;
    m_order = order;
    reset();
    delayReload();
}
void CommentsQuery::stop() {
    m_stopped = true;
    invalidateRequests();
    m_loading_more = false;
    if (m_store == m_owned_store) m_owned_store->clear();
    Q_EMIT changed();
}
void CommentsQuery::reload() {
    m_stopped = false;
    load(false);
}
void CommentsQuery::fetchMore() {
    if (! m_stopped && m_has_more && ! querying()) load(true);
}
void CommentsQuery::load(bool more) {
    if (! m_store || m_store->closed() || ! m_item_id.valid() ||
        (! m_parent_id.isEmpty() && m_store->scope().isEmpty()))
        return;
    invalidateRequests();
    auto revision      = beginRequest();
    auto read_revision = m_store->tick();
    auto generation    = m_store->generation();
    m_loading_more     = more;
    if (! more) {
        m_has_more = false;
        m_token.clear();
    }
    setError({});
    setStatus(Status::Querying);
    Q_EMIT changed();
    msg::GetCommentsReq req;
    req.setId_proto(m_item_id.id());
    req.setItemType(rstd::into(m_item_id.type()));
    req.setOrder(static_cast<msg::model::CommentOrderGadget::CommentOrder>(m_order));
    req.setLimit(20);
    if (more) req.setToken(m_token);
    auto self    = QWatcher { this };
    auto backend = App::instance()->backend();
    auto parent  = m_parent_id;
    auto scope   = m_store->scope();
    spawn([self, backend, req, parent, scope, revision, read_revision, generation, more]() mutable
              -> task<void> {
        auto fetch = [&]() -> task<Result<msg::model::CommentsResult, msg::Error>> {
            if (parent.isEmpty()) {
                auto rsp = co_await backend->send(std::move(req));
                if (! rsp) co_return Err(rsp.unwrap_err_unchecked());
                if (! rsp->hasResult())
                    co_return Err(msg::Error { 0, "Invalid comments response" });
                co_return Ok(rsp->result());
            }
            msg::GetCommentRepliesReq replies;
            replies.setQuery(req);
            replies.setParentId(parent);
            replies.setScope(scope);
            auto rsp = co_await backend->send(std::move(replies));
            if (! rsp) co_return Err(rsp.unwrap_err_unchecked());
            if (! rsp->hasResult()) co_return Err(msg::Error { 0, "Invalid comments response" });
            co_return Ok(rsp->result());
        };
        auto result = co_await fetch();
        if (! co_await QAsyncResult::qexecutor() || ! self || ! self->isCurrentRequest(revision) ||
            ! self->m_store || self->m_store->generation() != generation)
            co_return;
        self->m_loading_more = false;
        if (! result) {
            self->setError(QString::fromStdString(result.unwrap_err_unchecked().message));
            self->setStatus(Status::Error);
            Q_EMIT self->changed();
            co_return;
        }
        self->accept(*result, more, read_revision);
    });
}
void CommentsQuery::accept(const msg::model::CommentsResult& result, bool more, quint64 revision) {
    if (result.supported() && (! result.hasPage() || result.scope().isEmpty())) {
        setError(tr("Invalid comments response"));
        setStatus(Status::Error);
        Q_EMIT changed();
        return;
    }
    if (! more) m_model.reset(m_store);
    m_supported = result.supported();
    m_loaded    = true;
    if (m_supported) {
        const auto page = result.page();
        if (page.hasMore() && (! page.hasNextToken() || page.nextToken().isEmpty() ||
                               page.nextToken() == m_token || page.comments().isEmpty())) {
            setError(tr("Comment continuation did not advance"));
            setStatus(Status::Error);
            Q_EMIT changed();
            return;
        }
        m_store->bind(m_item_id, result);
        m_store->merge(page.comments(), revision);
        m_model.append(page.comments());
        m_token     = page.hasNextToken() ? page.nextToken() : QString();
        m_has_more  = page.hasMore();
        m_can_reply = page.canReply();
        m_total     = page.hasTotal() ? QVariant::fromValue(static_cast<qulonglong>(page.total()))
                                      : QVariant();
        m_orders.clear();
        for (auto order : page.orders()) m_orders.append(static_cast<int>(order));
    }
    setError({});
    setStatus(Status::Finished);
    Q_EMIT changed();
}
} // namespace qcm
#include "Qcm/query/comments_query.moc.cpp"
