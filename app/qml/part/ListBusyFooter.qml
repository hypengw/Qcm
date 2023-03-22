import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

Pane {
    property alias running: m_busy.running

    visible: running
    implicitHeight: m_busy.running ? m_busy.implicitHeight + 2 * padding : 0
    padding: 4
    clip: true

    BusyIndicator {
        id: m_busy

        anchors.centerIn: parent
    }

}
