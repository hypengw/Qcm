import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    icon.name: MD.Token.icon.add
    text: qsTr('create mix')
    onTriggered: {
        MD.Util.show_popup('qrc:/Qcm/App/qml/dialog/MixCreateDialog.qml', {}, QA.Global.main_win.Overlay.overlay);
    }
}
