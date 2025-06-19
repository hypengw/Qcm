#include "Qcm/model/page_model.hpp"

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
#ifndef NDEBUG
    self->insert(self->rowCount(),
                 Page { .name   = "test",
                        .icon   = "bug_report",
                        .source = "qrc:/Qcm/Material/Example/Example.qml" });
#endif
}
PageModel::PageModel(QObject* parent): base_type(parent) {}
PageModel::~PageModel() {}
} // namespace qcm

#include <Qcm/model/moc_page_model.cpp>