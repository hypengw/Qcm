module;
#include "Qcm/macro_qt.hpp"

#ifdef Q_MOC_RUN
#include "Qcm/model/app_info.moc"
#endif
export module qcm:model.app_info;
export import qextra;

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
