import QtQuick
import QtQuick.Controls.Basic

import Qcm.App as QA
import Qcm.Material as MD

Action {
    id: root
    icon.name: MD.Token.icon.playlist_add
    text: qsTr('add to list')
    onTriggered: {
        if (getSongIds) {
            QA.Action.queue_ids(root.getSongIds());
        }
    }
    property var getSongIds: null
}
