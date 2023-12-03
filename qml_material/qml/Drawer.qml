import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.Material
import QtQuick.Controls.Material.impl
import Qcm.Material as MD

T.Drawer {
    id: control

    parent: T.Overlay.overlay

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, contentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, contentHeight + topPadding + bottomPadding)

    topPadding: edge !== Qt.TopEdge ? 16 : 0
    bottomPadding: edge !== Qt.BottomEdge ? 16 : 0

    enter: Transition {
        SmoothedAnimation {
            velocity: 5
        }
    }
    exit: Transition {
        SmoothedAnimation {
            velocity: 5
        }
    }

    MD.MatProp.elevation: !interactive && !dim ? MD.Token.elevation.level0 : MD.Token.elevation.level1

    background: Item {
        implicitWidth: 200
        Rectangle {
            anchors.fill: parent
            color: MD.Token.color.surface
            layer.enabled: true
            layer.effect: ShaderEffect {
                property var radius: MD.Util.corner([0, 16, 0, 16]).toVector4D()
                property var size: parent.height
                fragmentShader: 'qrc:/Qcm/Material/assets/shader/round.frag.qsb'
            }
        }
        //layer.enabled: control.position > 0 && control.MD.MatProp.elevation > 0
        //layer.effect: MD.RoundedElevationEffect {
        //    elevation: control.MD.MatProp.elevation
        //}
    }

    T.Overlay.modal: Rectangle {
        color: MD.Util.transparent(MD.Token.color.scrim, 0.32)
        Behavior on opacity {
            NumberAnimation {
                duration: 150
            }
        }
    }

    T.Overlay.modeless: Rectangle {
        color: MD.Util.transparent(MD.Token.color.scrim, 0.32)
        Behavior on opacity {
            NumberAnimation {
                duration: 150
            }
        }
    }
}
