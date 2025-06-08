import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    id: root
    property QA.item_id itemId
    property QA.item_id sourceId

    enabled: root.itemId.valid && root.itemId !== QA.App.playqueue.currentSong.itemId
    icon.name: MD.Token.icon.play_arrow
    text: qsTr('play')

    onTriggered: {
        QA.Action.play(root.itemId);
    }
}
