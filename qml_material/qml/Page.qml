import QtQuick
import QtQuick.Templates as T
import Qcm.Material as MD

T.Page {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            contentWidth + leftPadding + rightPadding,
                            implicitHeaderWidth,
                            implicitFooterWidth)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding
                             + (implicitHeaderHeight > 0 ? implicitHeaderHeight + spacing : 0)
                             + (implicitFooterHeight > 0 ? implicitFooterHeight + spacing : 0))

    background: Rectangle {
        color: control.MD.MatProp.backgroundColor

        layer.enabled: control.enabled && control.MD.MatProp.elevation > 0
        layer.effect: MD.RoundedElevationEffect {
            elevation: control.MD.MatProp.Material.elevation
        }
    }
}