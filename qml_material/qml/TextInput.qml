import QtQuick
import Qcm.Material as MD

TextInput {
    id: root
    property QtObject typescale: MD.Token.typescale.body_large
    property bool prominent: false

    font.capitalization: Font.MixedCase

    Binding {
        when: typescale
        root.font.pixelSize: typescale.size
        root.font.weight: root.prominent && typescale.weight_prominent ? typescale.weight_prominent : typescale.weight
        root.font.letterSpacing: typescale.tracking
        restoreMode: Binding.RestoreNone
    }

    cursorDelegate: MD.CursorDelegate {
    }

    color: MD.MatProp.textColor
    selectionColor: MD.Token.color.primary
    selectedTextColor: MD.Token.color.getOn(selectionColor)

    verticalAlignment: TextInput.AlignVCenter
}
