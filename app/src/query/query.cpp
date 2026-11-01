module qcm;
import :query.query;
import :app;
import :status.provider;
import :msg;

namespace qcm
{

void connectSyncFinished(Query* q) {
    auto backend = App::instance()->backend();
    auto reload  = [q, backend] {
        if (! backend->resource_base_url().isEmpty()) q->delayReload();
    };
    QObject::connect(AppStore::instance(), &AppStore::stateChanged, q, reload);
    QObject::connect(AppStore::instance(), &AppStore::scopeChanged, q, reload);
    QObject::connect(backend, &Backend::resourceStateChanged, q, reload);
    q->connect(App::instance()->provider_status(),
               &ProviderStatusModel::syncingChanged,
               q,
               [q](bool syncing) {
                   if (syncing) return;
                   q->delayReload();
               });
}

} // namespace qcm
