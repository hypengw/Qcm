import QtQuick

import Qcm.App as QA
import Qcm.Msg as QM
import Qcm.Material as MD

MD.Action {
    id: root
    property QM.item_id itemId
    property QM.item_id sourceId

    enabled: root.itemId.valid && root.itemId !== QA.App.playqueue.currentSong.itemId()
    icon.name: MD.Token.icon.play_arrow
    text: qsTr('play next')

    onTriggered: {
        const ids = [root.itemId];
        QA.Action.queue_next(ids, sourceId);
    }
}
