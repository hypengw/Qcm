module;
#include "Qcm/macro.hpp"
#include "Qcm/macro_qt.hpp"


#ifdef Q_MOC_RUN
#include "Qcm/model/page_model.moc"
#endif

export module qcm:model.page_model;
export import qcm.qt;

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