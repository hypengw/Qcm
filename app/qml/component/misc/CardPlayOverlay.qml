import QtQuick
import Qcm.Material as MD
import Qcm.App as QA

QA.CardOverlay {
    id: root
    property QA.item_id itemId
    property MD.Action playAction: QA.PlayAction {
        itemId: root.itemId
        icon.name: MD.Token.icon.play_arrow
        checked: true
    }
    property bool smallButton: false
    property int buttonAlignment: Qt.AlignCenter

    MD.ActionIconButton {
        id: m_button
        width: root.smallButton ? 32 + leftInset + rightInset : implicitWidth
        height: root.smallButton ? 32 + topInset + bottomInset : implicitHeight
        x: root.buttonAlignment & Qt.AlignRight ? parent.width - width - 8
            : root.buttonAlignment & Qt.AlignLeft ? 8 : (parent.width - width) / 2
        y: root.buttonAlignment & Qt.AlignBottom ? parent.height - height - 8
            : root.buttonAlignment & Qt.AlignTop ? 8 : (parent.height - height) / 2
        action: root.playAction
        icon.width: root.smallButton ? 20 : 30
        icon.height: root.smallButton ? 20 : 30
        padding: root.smallButton ? 4 : 14
        mdState.selected: true
        icon.fill: true
        mdState.type: MD.Enum.IBtFilled
        mdState.isRound: !root.smallButton
        mdState.size: root.smallButton ? MD.Enum.XS : MD.Enum.S
    }
}
