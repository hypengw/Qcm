import QtQuick
import Qcm.Material as MD

MD.EmbedChip {
    property bool asc: true
    text: "order"
    leftPadding: 8
    trailingIconName: asc ? MD.Token.icon.arrow_upward : MD.Token.icon.arrow_downward
}
