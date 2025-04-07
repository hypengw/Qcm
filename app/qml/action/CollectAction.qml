import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    id: root
    property bool liked: false// QA.Global.session.user.collection.contains(itemId)
    property var itemId: null

    icon.name: liked ? MD.Token.icon.done : MD.Token.icon.add
    text: qsTr(liked ? 'favorited' : 'favorite')

    onTriggered: {
        QA.Action.collect(root.itemId, !liked);
    }
}
