import QtQuick
import QtQuick.Controls

import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

Action {
    required property var itemId
    icon.name: MD.Token.icon.comment
    text: qsTr('comment')
    onTriggered: {
        QA.Global.show_page_popup('qrc:/Qcm/App/qml/page/CommentPage.qml', {
                "itemId": itemId
            });
    }
}
