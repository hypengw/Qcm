import QtQuick
import Qcm.Material as MD

MD.RectangularGlow {
    property int offsetX
    property int offsetY
    property int blurRadius
    property int spreadRadius

    property real strength

    property Item source

    property bool fullWidth
    property bool fullHeight

    readonly property real sourceRadius: source && source.radius || 0

    x: (parent.width - width)/2 + offsetX
    y: (parent.height - height)/2 + offsetY

    implicitWidth: source ? source.width : parent.width
    implicitHeight: source ? source.height : parent.height

    width: implicitWidth + 2 * spreadRadius + (fullWidth ? 2 * cornerRadius : 0)
    height: implicitHeight + 2 * spreadRadius + (fullHeight ? 2 * cornerRadius : 0)
    glowRadius: blurRadius/2
    spread: strength

    cornerRadius: blurRadius + sourceRadius
}