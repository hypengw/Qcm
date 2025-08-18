import QtQuick
import Qcm.Material as MD

Flow {
    id: root
    property var filter
    signal clicked
    MD.InputChip {
        text: qsTr('empty')
        onClicked: root.clicked()
    }
}
