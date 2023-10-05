import QtQuick
import Qcm.Material as MD

Item {
    id: root
    implicitWidth: initialSize
    implicitHeight: initialSize

    property real value: 0
    property bool handleHasFocus: false
    property bool handlePressed: false
    property bool handleHovered: false
    readonly property int initialSize: 4
    readonly property var control: parent

    Rectangle {
        id: handleRect
        anchors.centerIn: parent
        width: 20; height: 20
        radius: width / 2
        color: root.control
            ? root.control.MD.MatProp.backgroundColor
            : "transparent"

        layer.enabled: true
        layer.effect: MD.RoundedElevationEffect {
            elevation: MD.Token.elevation.level1
        }
    }

    MD.Ripple {
        anchors.centerIn: parent
        width: 28; height: 28
        pressed: root.handlePressed
        active: root.handlePressed || root.handleHasFocus || (enabled && root.handleHovered)
        color: root.control ? MD.Util.transparent(root.control.MD.MatProp.backgroundColor, MD.Token.state.hover.state_layer_opacity) : "transparent"
    }
}