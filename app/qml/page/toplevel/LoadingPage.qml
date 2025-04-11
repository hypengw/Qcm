import QtQuick
import Qcm.Material as MD

MD.Page {
    Column {
        anchors.centerIn: parent
        spacing: 12
        MD.IconSvg {
            anchors.horizontalCenter: parent.horizontalCenter
            source: 'qrc:/Qcm/App/assets/Qcm.svg'
            sourceSize: Qt.size(width, height)
            width: 96
            height: 96
        }
        MD.CircularIndicator {
            anchors.horizontalCenter: parent.horizontalCenter
            running: visible
        }
    }
}
