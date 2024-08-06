import QtQuick
import QtQuick.Controls

import Qcm.App as QA
import Qcm.Material as MD

Action {
    icon.name: MD.Token.icon.playlist_add
    text: qsTr('add to list')
    onTriggered: if(getSongs) QA.Global.appendList(this.getSongs())
    property var getSongs: null
}
