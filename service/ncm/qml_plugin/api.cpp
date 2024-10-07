#include "service_qml_ncm/api.h"

#include <QUuid>
#include "qcm_interface/global.h"

namespace qcm
{

ncm::Client detail::get_client() {
    return ncm::Client(
        Global::instance()->session(),
        Global::instance()->pool_executor(),
        ncm::api::device_id_from_uuid(Global::instance()->uuid().toString().toStdString()));
}

} // namespace qcm