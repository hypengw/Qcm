module;
#include <QtQmlIntegration/QtQmlIntegration>
#include "Qcm/model/app_info.moc.h"

#ifdef Q_MOC_RUN
#include "Qcm/model/app_info.moc"
#endif
export module qcm.model.app_info;

export namespace qcm::model
{

class AppInfo {
    Q_GADGET
    QML_VALUE_TYPE(app_info)
public:
    AppInfo();
    ~AppInfo();

    Q_PROPERTY(QString id MEMBER id CONSTANT FINAL)
    Q_PROPERTY(QString name MEMBER name CONSTANT FINAL)
    Q_PROPERTY(QString version MEMBER version CONSTANT FINAL)
    Q_PROPERTY(QString author MEMBER author CONSTANT FINAL)
    Q_PROPERTY(QString summary MEMBER summary CONSTANT FINAL)
private:
    QString id;
    QString name;
    QString version;
    QString author;
    QString summary;
};


} // namespace qcm::model

module :private;

namespace qcm::model
{
AppInfo::AppInfo() {
    this->name    = APP_NAME;
    this->id      = APP_ID;
    this->author  = APP_AUTHOR;
    this->summary = APP_SUMMARY;
    this->version = APP_VERSION;
}
AppInfo::~AppInfo() {}
} // namespace qcm::model

#include "Qcm/model/app_info.moc.cpp"