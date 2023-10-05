import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Pane {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    padding: 12
    property real radius
    MD.MatProp.elevation: MD.Token.elevation.level0

    background: Rectangle {
        color: control.MD.MatProp.backgroundColor
        radius: control.radius

        layer.enabled: control.enabled && control.MD.MatProp.elevation > 0
        layer.effect: MD.RoundedElevationEffect {
            elevation: control.MD.MatProp.elevation
        }
    }
}