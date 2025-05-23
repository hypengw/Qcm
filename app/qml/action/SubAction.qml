import QtQuick
import QtQml

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    id: root
    property bool liked: false
    property var itemId: null
    property QtObject querier: null

    icon.name: liked ? MD.Token.icon.done : MD.Token.icon.add
    text: qsTr(liked ? 'favorited' : 'favorite')
    onTriggered: {
        querier.sub = !liked;
        querier.itemId = root.itemId;
        querier.query();
    }
    Binding on liked {
        value: root.querier.sub
        when: root.querier.status === QA.Enum.Finished
    }
}
