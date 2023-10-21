import QtQuick
import QtQuick.Controls
import Qcm.Material as MD

MD.Pane {
    property alias running: m_busy.running

    visible: running
    implicitHeight: m_busy.running ? m_busy.implicitHeight + 2 * padding : 0
    padding: 4
    clip: true

    MD.CircularIndicator {
        id: m_busy

        anchors.centerIn: parent
    }
}
