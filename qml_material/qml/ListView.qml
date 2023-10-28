import QtQuick
import Qcm.Material as MD

ListView {
    id: root

    property bool busy: false

    footer: MD.ListBusyFooter {
        running: root.busy
        width: ListView.view.width
    }

    signal wheelMoved

    MD.WheelHandler {
        id: wheel
        target: root
        filterMouseEvents: false
        onWheelMoved: root.wheelMoved()
    }

    ScrollBar.vertical: MD.ScrollBar {
    }
}
