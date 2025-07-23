#pragma once
#include <QQmlEngine>
#include "core/core.h"
#include "kstore/qt/gadget_model.hpp"
#include "Qcm/macro.hpp"

namespace qcm
{

class Page {
    Q_GADGET
    QML_ANONYMOUS
public:
    GADGET_PROPERTY_DEF(QString, name, name)
    GADGET_PROPERTY_DEF(QString, icon, icon)
    GADGET_PROPERTY_DEF(QString, source, source)
    GADGET_PROPERTY_DEF(bool, cache, cache)
    GADGET_PROPERTY_DEF(bool, primary, primary)
};

class PageModel : public kstore::QGadgetListModel,
                  public kstore::QMetaListModelCRTP<Page, PageModel, kstore::ListStoreType::Vector> {
    Q_OBJECT
    QML_ANONYMOUS

public:
    PageModel(QObject* parent = nullptr);
    ~PageModel();

    static void init_main_pages(PageModel*);
};

} // namespace qcm