#include "service_qml_ncm/session.h"
#include "service_qml_ncm/client.h"

#define QRC_URL  u"qrc:/Qcm/Service/Ncm/"
#define PAGE_URL QRC_URL u"qml/page/"

namespace ncm::qml
{
namespace
{
auto main_pages() -> std::vector<qcm::model::Page> {
    return {
        qcm::model::Page { "library",
                           "library_music",
                           QStringLiteral("qrc:/Qcm/App/qml/page/MinePage.qml"),
                           true,
                           true },
        qcm::model::Page { "today", "today", QStringLiteral(PAGE_URL "TodayPage.qml"), true, true },
        qcm::model::Page { "playlist",
                           "queue_music",
                           QStringLiteral(PAGE_URL "PlaylistListPage.qml"),
                           false,
                           true },
        qcm::model::Page {
            "cloud", "cloud", QStringLiteral(PAGE_URL "CloudPage.qml"), true, false },
        //        qcm::model::Page {
        //            "history", "history", QStringLiteral(PAGE_URL "RecordPage.qml"), false, false
        //            },
    };
}
} // namespace

Session::Session(QObject* parent): qcm::model::Session(parent) {
    set_valid(true);
    set_pages(main_pages());
    set_client(ncm::qml::create_client());
    set_supportComment(true);
}
Session::~Session() {}

} // namespace ncm::qml
