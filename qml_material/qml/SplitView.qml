import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import Qcm.Material as MD

T.SplitView {
    id: control
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    handle: Rectangle {
        implicitWidth: control.orientation === Qt.Horizontal ? 6 : control.width
        implicitHeight: control.orientation === Qt.Horizontal ? control.height : 6
        color: T.SplitHandle.pressed ? control.MD.MatProp.backgroundColor : Qt.lighter(control.MD.MatProp.backgroundColor, T.SplitHandle.hovered ? 1.1 : 1.0)

        Rectangle {
            color: control.MD.MatProp.textColor
            width: control.orientation === Qt.Horizontal ? thickness : length
            height: control.orientation === Qt.Horizontal ? length : thickness
            radius: thickness
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2

            property int length: parent.T.SplitHandle.pressed ? 3 : 8
            readonly property int thickness: parent.T.SplitHandle.pressed ? 3 : 1

            Behavior on length  {
                NumberAnimation {
                    duration: 100
                }
            }
        }
    }
}
