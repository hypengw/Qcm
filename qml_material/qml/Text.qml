import QtQuick
import Qcm.Material as MD

Text {
    id: root
    property QtObject typescale: MD.Token.typescale.label_medium
    property bool prominent: false

    Binding {
        when: typescale
        root.lineHeight: typescale ? typescale?.line_height : 16
        root.font.pixelSize: typescale ? typescale?.size : 16
        root.font.weight: typescale ? (root.prominent && typescale?.weight_prominent ? typescale?.weight_prominent : typescale?.weight) : Font.Normal
        root.font.letterSpacing: typescale ? typescale.tracking : 1
        restoreMode: Binding.RestoreNone
    }

    color: MD.MatProp.textColor
    lineHeightMode: Text.FixedHeight
    wrapMode: Text.Wrap
    elide: Text.ElideRight
    maximumLineCount: 1
    textFormat: Text.PlainText
}
