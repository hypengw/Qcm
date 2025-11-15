import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Dialog {
    id: root
    title: qsTr('Link A Mix')
    standardButtons: MD.Dialog.Ok | MD.Dialog.Cancel

    onAccepted: {}

    ColumnLayout {
        anchors.fill: parent
        spacing: 24
    }
}
