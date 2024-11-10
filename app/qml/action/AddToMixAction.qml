import QtQuick
import QtQuick.Controls.Basic

import Qcm.App as QA
import Qcm.Material as MD

Action {
    id: root
    icon.name: MD.Token.icon.queue
    text: qsTr('add to mix')
    required property QA.t_id songId
    onTriggered: {
        MD.Util.show_popup('qrc:/Qcm/App/qml/dialog/AddToMixDialog.qml', {
            "songId": root.songId
        }, QA.Global.main_win.Overlay.overlay);
    }
}
