import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.TextField {
    id: root
    property QtObject typescale: MD.Token.typescale.body_large
    property bool prominent: false

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    Binding {
        when: typescale
        root.font.pixelSize: typescale.size
        root.font.weight: root.prominent && typescale.weight_prominent ? typescale.weight_prominent : typescale.weight
        root.font.letterSpacing: typescale.tracking
        restoreMode: Binding.RestoreNone
    }

    cursorDelegate: MD.CursorDelegate {
    }
    selectionColor: MD.Token.color.primary
    selectedTextColor: MD.Token.color.getOn(selectionColor)

    verticalAlignment: TextInput.AlignVCenter
    color: MD.MatProp.textColor
}
