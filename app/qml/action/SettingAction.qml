import QtQuick
import QtQuick.Controls
import Qcm.App as QA
import Qcm.Material as MD

Action {
    id: root
    icon.name: MD.Token.icon.settings
    text: qsTr('settings')

    onTriggered: {
        QA.Action.popup_special(QA.enums.SRSetting);
    }
}
