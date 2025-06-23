import QtQuick
import Qcm.Material as MD
import Qcm.App as QA

QA.ImageCard {
    id: root

    property alias itemId: m_overlay.itemId

    QA.CardPlayOverlay {
        id: m_overlay
        height: root.picWidth
        width: root.picWidth
        corners: MD.Util.corners(root.mdState.radius)
        opacity: root.hovered ? 1 : 0

        Behavior on opacity {
            NumberAnimation {
                duration: MD.Token.duration.medium2
                easing: MD.Token.easing.standard
            }
        }
    }
}
