import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.Action {
    id: root
    icon.name: MD.Token.icon.settings
    text: qsTr('settings')

    onTriggered: {
        QA.Action.openPopup(QA.Enum.SRSetting);
    }
}
