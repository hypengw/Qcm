import QtQuick
import Qcm.Material as MD
import Qcm.App as QA

QA.CardOverlay {
    id: root
    property QA.item_id itemId

    MD.IconButton {
        anchors.centerIn: parent
        action: QA.PlayAction {
            itemId: root.itemId
            icon.name: MD.Token.icon.play_arrow
            icon.width: 30
            icon.height: 30
            checked: true
        }
        padding: 14
        checked: true
        type: MD.Enum.BtFilled
    }
}
