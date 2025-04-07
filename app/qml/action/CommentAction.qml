import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    id: root
    required property var itemId
    icon.name: MD.Token.icon.comment
    text: qsTr('comment')
    onTriggered: {
        QA.Action.popup_page('qrc:/Qcm/App/qml/page/CommentPage.qml', {
                "itemId": root.itemId
            });
    }
}
