import QtQuick
import QtQuick.Controls

import Qcm.App as QA
import Qcm.Material as MD

Action {
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
        value: querier.sub
        when: querier.status === QA.ApiQuerierBase.Finished
    }
}
