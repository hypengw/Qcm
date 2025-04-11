import QtQuick
import Qcm.App as QA
import Qcm.Material as MD

MD.Drawer {
    id: root
    font.capitalization: Font.Capitalize
    padding: 0

    MD.MProp.backgroundColor: MD.MProp.color.surface

    contentItem: QA.DrawerContent {}
    Connections {
        target: root.contentItem
        function onClose() {
            root.close();
        }
    }

    Connections {
        target: QA.Action
        function onOpen_drawer() {
            root.open();
        }
    }
}
