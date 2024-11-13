#pragma once

#include "qcm_interface/model/id_queue.h"
#include "qcm_interface/async.h"

namespace ncm::qml
{

class FmQueue : public qcm::model::IdQueue {
    Q_OBJECT
public:
    FmQueue(QObject* parent = nullptr);
    ~FmQueue();

    Q_SLOT void onRequestNext();

private:
    qcm::QAsyncResult* m_query;
};

} // namespace ncm::qml