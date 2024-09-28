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
        QA.Action.popup_page('qrc:/Qcm/Service/Ncm/qml/page/CommentPage.qml', {
                "itemId": itemId
            });
    }
}
