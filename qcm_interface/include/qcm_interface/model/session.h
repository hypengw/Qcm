#pragma once
#include <QObject>
#include "qcm_interface/model.h"
#include "qcm_interface/model/user_model.h"
#include "qcm_interface/model/page.h"
#include "qcm_interface/client.h"

namespace request
{
class Request;
};

namespace qcm::model
{

class QCM_INTERFACE_API Session : public Model<Session, QObject> {
    Q_OBJECT
    DECLARE_MODEL()
public:
    Session(QObject* parent = nullptr);
    ~Session();

    DECLARE_PROPERTY(qcm::model::UserAccount*, user, NOTIFY_NAME(userChanged))
    DECLARE_PROPERTY(std::vector<Page>, pages, NOTIFY_NAME(infoChanged))
    DECLARE_PROPERTY(bool, valid, NOTIFY_NAME(infoChanged))
    DECLARE_PROPERTY(QString, provider, NOTIFY_NAME(infoChanged))
    DECLARE_PROPERTY(bool, supportComment, NOTIFY_NAME(infoChanged))

    Q_SIGNAL void infoChanged();
    Q_SIGNAL void userChanged();

    auto client() const -> std::optional<Client>;
    void set_client(std::optional<Client>);

private:
    std::optional<Client> m_client;
};

} // namespace qcm::model