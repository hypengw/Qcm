import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QcmApp

ListView {
    id: root
    signal wheelMoved

    FlickableScrollHandler {
        onMoved: root.wheelMoved()
    }
}
