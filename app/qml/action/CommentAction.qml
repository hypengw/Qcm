import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    id: root
    required property var itemId
    icon.name: MD.Token.icon.comment
    text: qsTr('comment')
    onTriggered: {
        const msg = Util.routeMsg();
        msg.dst = 'qrc:/Qcm/App/qml/page/CommentPage.qml';
        msg.props = {
            itemId: root.itemId
        };
        QA.Action.openPopup(msg);
    }
}
