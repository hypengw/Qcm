module qcm;
import :query.query;
import :app;
import :status.provider;

namespace qcm
{

void connectSyncFinished(Query* q) {
    q->connect(App::instance()->provider_status(),
               &ProviderStatusModel::syncingChanged,
               q,
               [q](bool syncing) {
                   if (syncing) return;
                   q->delayReload();
               });
}

} // namespace qcm
