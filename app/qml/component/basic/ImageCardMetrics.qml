pragma Singleton
import QtQuick
import Qcm.Material as MD

MD.TextMetrics {
    typescale: MD.Token.typescale.body_medium
    text: "Ag"

    readonly property int labelHeight: Math.ceil(height)
    readonly property int imageSpacing: 8
    readonly property int labelSpacing: 4
    readonly property int bottomPadding: 8
    readonly property int extraHeight: imageSpacing + labelHeight * 2 + labelSpacing + bottomPadding
}
