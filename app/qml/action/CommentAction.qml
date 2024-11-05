import QtQuick
import QtQuick.Controls.Basic

import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

Action {
    required property var itemId
    icon.name: MD.Token.icon.comment
    text: qsTr('comment')
    onTriggered: {
        QA.Action.popup_page('qrc:/Qcm/App/qml/page/CommentPage.qml', {
                "itemId": itemId
            });
    }
}
