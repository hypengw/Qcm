import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.Material.impl as MDImpl

import Qcm.Material as MD

T.BusyIndicator {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 6

    contentItem: MDImpl.BusyIndicatorImpl {
        implicitWidth: 48
        implicitHeight: 48
        color: control.MD.MatProp.color.primary

        running: control.running
        opacity: control.running ? 1 : 0
        Behavior on opacity { OpacityAnimator { duration: 250 } }
    }
}