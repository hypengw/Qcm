#pragma once
#include <QQmlEngine>
#include "core/core.h"
#include "meta_model/qgadget_list_model.hpp"
#include "qcm_interface/macro.h"


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

class PageModel : public meta_model::QGadgetListModel<Page> {
    Q_OBJECT
    QML_ANONYMOUS
    using base_type = meta_model::QGadgetListModel<Page>;

public:
    PageModel(QObject* parent = nullptr);
    ~PageModel();

    static void init_main_pages(PageModel*);
};

} // namespace qcm