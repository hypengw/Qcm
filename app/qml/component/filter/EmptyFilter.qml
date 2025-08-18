import QtQuick
import Qcm.Material as MD

Flow {
    id: root
    signal clicked
    MD.InputChip {
        text: qsTr('empty')
        onClicked: root.clicked()
    }
}
