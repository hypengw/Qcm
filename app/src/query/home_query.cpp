module;
#include "Qcm/query/home_query.moc.h"
module qcm;
import :query.home;
import :app;
import :queue;

namespace qcm
{
HomeRecommendationsQuery::HomeRecommendationsQuery(QObject* parent)
    : Query(parent), m_for_you(this), m_radios(this), m_play(this) {
    auto app       = App::instance();
    auto providers = app->provider_status();
    connect(providers,
            &ProviderStatusModel::activedIdsChanged,
            this,
            &HomeRecommendationsQuery::reconcileProviders);
    connect(providers, &QAbstractItemModel::dataChanged, this, [this] {
        reconcileProviders();
    });
    connect(providers, &ProviderStatusModel::providerSyncFinished, this, [this](qint64) {
        invalidate();
        delayReload();
    });
    connect(providers, &ProviderStatusModel::syncingChanged, this, [this](bool syncing) {
        invalidate();
        if (! syncing) delayReload();
    });
    connect(app->backend(), &Backend::disconnected, this, [this] {
        invalidate();
        m_snapshots.clear();
        m_epochs.clear();
        apply();
    });
    connect(app->backend(), &Backend::connected, this, [this] {
        reconcileProviders();
        delayReload();
    });
    reconcileProviders();
}
auto HomeRecommendationsQuery::forYou() -> model::ForYouListModel* { return &m_for_you; }
auto HomeRecommendationsQuery::radios() -> model::RadioQueueListModel* { return &m_radios; }
void HomeRecommendationsQuery::invalidate() {
    ++m_revision;
    cancel();
}
void HomeRecommendationsQuery::reconcileProviders() {
    auto                  providers = App::instance()->provider_status();
    auto                  ids       = providers->activedIds();
    QMap<qint64, quint64> epochs;
    for (auto id : ids) {
        auto value = providers->itemById({ enums::ItemType::ItemProvider, id })
                         .value<model::ProviderStatus>();
        epochs.insert(id, value.epoch());
    }
    if (ids == m_provider_ids && epochs == m_epochs) return;
    invalidate();
    for (auto it = m_snapshots.begin(); it != m_snapshots.end();) {
        if (! epochs.contains(it.key()) || m_epochs.value(it.key()) != epochs.value(it.key())) {
            if (epochs.contains(it.key())) {
                for (const auto& radio : it->radios()) radioQueue(radio)->invalidateSession();
                for (const auto& card : it->forYou())
                    if (card.hasRadio()) radioQueue(card.radio())->invalidateSession();
            }
            it = m_snapshots.erase(it);
        } else
            ++it;
    }
    m_provider_ids = std::move(ids);
    m_epochs       = std::move(epochs);
    apply();
    delayReload();
}
void HomeRecommendationsQuery::apply() {
    QList<model::ForYouCard> cards;
    QList<model::RadioQueue> radios;
    for (auto id : m_provider_ids) {
        if (! m_snapshots.contains(id)) continue;
        const auto& snapshot = m_snapshots[id];
        cards.append(snapshot.forYou());
        radios.append(snapshot.radios());
    }
    m_for_you.sync(cards);
    m_radios.resetModel(radios);
}
void HomeRecommendationsQuery::reload() {
    reconcileProviders();
    setStatus(Status::Querying);
    auto                           backend = App::instance()->backend();
    msg::GetHomeRecommendationsReq req;
    req.setProviderIds(m_provider_ids);
    auto self     = QWatcher { this };
    auto revision = ++m_revision;
    spawn([self, backend, req, revision] mutable -> task<void> {
        auto rsp = co_await backend->send(std::move(req));
        if (! co_await QAsyncResult::qexecutor()) co_return;
        if (! self || self->m_revision != revision) co_return;
        if (! rsp) {
            self->setError(QString::fromStdString(rsp.unwrap_err_unchecked().message));
            self->setStatus(Status::Error);
            co_return;
        }
        QStringList                     errors;
        QMap<qint64, model::RadioQueue> prepare;
        for (const auto& candidate : rsp->providers()) {
            auto id = candidate.providerId();
            if (! self->m_epochs.contains(id) || self->m_epochs[id] != candidate.epoch()) continue;
            if (! candidate.error().isEmpty()) {
                errors.append(candidate.error());
                continue;
            }
            for (const auto& diagnostic : candidate.diagnostics())
                qWarning().noquote() << "home:" << id << diagnostic;
            self->m_snapshots.insert(id, candidate);
            for (const auto& card : candidate.forYou())
                if (card.hasRadio()) prepare.insert(card.radio().id_proto(), card.radio());
            for (const auto& radio : candidate.radios()) prepare.insert(radio.id_proto(), radio);
        }
        self->apply();
        for (const auto& radio : prepare) self->radioQueue(radio)->ensureStarted();
        self->setError(errors.join(u'\n'));
        self->setStatus(errors.isEmpty() ? Status::Finished : Status::Error);
        co_return;
    });
}
auto HomeRecommendationsQuery::radioQueue(const model::RadioQueue& radio)
    -> model::DynamicIdQueue* {
    return App::instance()->playqueue()->dynamicQueue(model::item_id(radio),
                                                      radio.name(),
                                                      radio.configuration(),
                                                      radio.canRestart(),
                                                      radio.restartLabel());
}
void HomeRecommendationsQuery::activateRadio(const model::RadioQueue& radio) {
    Action::instance()->switch_queue(radioQueue(radio));
}
void HomeRecommendationsQuery::activate(const model::ForYouCard& card, bool playButton) {
    using A     = msg::model::CardActionGadget::CardAction;
    auto action = playButton ? card.playAction() : card.primaryAction();
    auto id     = model::item_id(card);
    switch (action) {
    case A::CARD_ACTION_OPEN_ITEM: Action::instance()->routeItem(id); break;
    case A::CARD_ACTION_PLAY_ITEM:
        m_play.setItemId(id);
        m_play.reload();
        break;
    case A::CARD_ACTION_ACTIVATE_RADIO:
        if (card.hasRadio()) activateRadio(card.radio());
        break;
    default: break;
    }
}
} // namespace qcm
#include "Qcm/query/home_query.moc.cpp"
