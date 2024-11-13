#pragma once

#include "qcm_interface/model/id_queue.h"
#include "qcm_interface/async.h"

namespace ncm::qml
{

class FmQueue : public qcm::model::IdQueue {
    Q_OBJECT

    Q_PROPERTY(QString name READ name CONSTANT FINAL)
public:
    FmQueue(QObject* parent = nullptr);
    ~FmQueue();

    auto name() const -> const QString&;

    Q_SLOT void onRequestNext();

private:
    qcm::QAsyncResult* m_query;
    QString            m_name;
};

} // namespace ncm::qml