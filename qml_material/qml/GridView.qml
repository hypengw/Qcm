import QtQuick
import QtQuick.Controls
import Qcm.Material as MD

GridView {
    id: root

    signal wheelMoved
    MD.WheelHandler {
        id: wheel
        target: root
        filterMouseEvents: false
        onWheelMoved: root.wheelMoved()
    }
}
