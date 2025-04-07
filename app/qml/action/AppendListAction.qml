import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    id: root
    icon.name: MD.Token.icon.playlist_add
    text: qsTr('add to list')
    onTriggered: {
        if (getSongIds) {
            QA.Action.queue_ids(root.getSongIds());
        } else if(getSongs) {
            QA.Action.queue(root.getSongs());
        }
    }
    property var getSongIds: null
    property var getSongs: null
}
