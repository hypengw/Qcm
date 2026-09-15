import QtQuick
import Qcm.Material as MD
import Qcm.App as QA

QA.CardOverlay {
    id: root
    property QA.item_id itemId
    property bool smallButton: false
    property int buttonAlignment: Qt.AlignCenter

    MD.IconButton {
        id: m_button
        x: root.buttonAlignment & Qt.AlignRight ? parent.width - width - 8
            : root.buttonAlignment & Qt.AlignLeft ? 8 : (parent.width - width) / 2
        y: root.buttonAlignment & Qt.AlignBottom ? parent.height - height - 8
            : root.buttonAlignment & Qt.AlignTop ? 8 : (parent.height - height) / 2
        action: QA.PlayAction {
            itemId: root.itemId
            icon.name: MD.Token.icon.play_arrow
            icon.width: root.smallButton ? m_button.mdState.iconSize : 30
            icon.height: root.smallButton ? m_button.mdState.iconSize : 30
            checked: true
        }
        padding: root.smallButton ? 4 : 14
        checked: true
        mdState.type: MD.Enum.IBtFilled
        mdState.size: root.smallButton ? MD.Enum.XS : MD.Enum.S
    }
}
