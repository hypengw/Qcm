module;
#include <QQmlEngine>
#include "kstore/qt/gadget_model.hpp"
#include "Qcm/macro.hpp"

#include "Qcm/model/page_model.moc.h"

#ifdef Q_MOC_RUN
#include "Qcm/model/page_model.moc"
#endif

export module qcm.model.page_model;
export import qcm.core;

namespace qcm
{

export class Page {
    Q_GADGET
    QML_ANONYMOUS
public:
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, icon, icon)
    GADGET_PROPERTY_DEF(QString, source, source)
    GADGET_PROPERTY_DEF(bool, cache, cache)
    GADGET_PROPERTY_DEF(bool, primary, primary)
};

export class PageModel
    : public kstore::QGadgetListModel,
      public kstore::QMetaListModelCRTP<Page, PageModel, kstore::ListStoreType::Vector> {
    Q_OBJECT
    QML_ANONYMOUS

public:
    PageModel(QObject* parent = nullptr);
    ~PageModel();

    static void init_main_pages(PageModel*);
};

} // namespace qcm
module :private;

namespace qcm
{

void PageModel::init_main_pages(PageModel* self) {
    std::array arr { Page { .name   = "home",
                            .icon   = "home",
                            .source = "qrc:/Qcm/App/qml/page/HomePage.qml",
                            .cache  = true },
                     Page { .name   = "library",
                            .icon   = "library_music",
                            .source = "qrc:/Qcm/App/qml/page/LibraryPage.qml",
                            .cache  = true },
                     Page { .name   = "search",
                            .icon   = "search",
                            .source = "qrc:/Qcm/App/qml/page/SearchPage.qml" } };
    self->insert(0, arr);
#ifdef QCM_DEBUG_BUILD
    self->insert(self->rowCount(),
                 Page { .name   = "test",
                        .icon   = "bug_report",
                        .source = "qrc:/Qcm/Material/Example/Example.qml" });
#endif
}
PageModel::PageModel(QObject* parent): kstore::QGadgetListModel(this, parent) {}
PageModel::~PageModel() {}
} // namespace qcm

#include "Qcm/model/page_model.moc.cpp"