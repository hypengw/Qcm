pragma ValueTypeBehavior: Addressable
import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    id: root
    required property var itemId
    icon.name: MD.Token.icon.comment
    text: qsTr('comment')
    onTriggered: {
        const msg = {
            dst: 'qrc:/Qcm/App/qml/page/CommentPage.qml',
            props: {
                itemId: root.itemId
            }
        } as QA.rmsg;
        QA.Action.openPopup(msg);
    }
}
