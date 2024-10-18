import QtQuick
import QtQuick.Controls.Basic

import Qcm.App as QA
import Qcm.Material as MD

Action {
    icon.name: MD.Token.icon.playlist_add
    text: qsTr('add to list')
    onTriggered: if(getSongs) {
        QA.Action.queue_songs(this.getSongs());
    }
    property var getSongs: null
}
