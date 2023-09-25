import QtQuick
import Qcm.Material as MD

Text {
    id: root
    property QtObject typescale: MD.Token.typescale.label_medium

    Binding {
        when: typescale
        root.lineHeight: typescale.line_height
        root.font.pixelSize: typescale.size
        root.font.weight: typescale.weight
        root.font.letterSpacing: typescale.tracking
    }

    color: MD.MatProp.textColor
    lineHeightMode: Text.FixedHeight
    wrapMode: Text.Wrap
    elide: Text.ElideRight
    maximumLineCount: 1
    textFormat: Text.PlainText
}