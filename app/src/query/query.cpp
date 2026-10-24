module qcm;
import :query.query;
import :app;
import :status.provider;
import :msg;

namespace qcm
{

void connectSyncFinished(Query* q) {
    QObject::connect(AppStore::instance(), &AppStore::stateChanged, q, &Query::delayReload);
    QObject::connect(AppStore::instance(), &AppStore::scopeChanged, q, &Query::delayReload);
    QObject::connect(App::instance()->backend(), &Backend::connected, q, &Query::delayReload);
    q->connect(App::instance()->provider_status(),
               &ProviderStatusModel::syncingChanged,
               q,
               [q](bool syncing) {
                   if (syncing) return;
                   q->delayReload();
               });
}

} // namespace qcm
