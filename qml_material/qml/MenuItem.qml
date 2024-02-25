import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import Qcm.Material as MD

T.MenuItem {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding, implicitIndicatorHeight + topPadding + bottomPadding)

    padding: 16
    verticalPadding: 0
    spacing: 16

    icon.width: 24
    icon.height: 24
    icon.color: MD.MatProp.textColor

    /*
    indicator: CheckIndicator {
        x: control.text ? (control.mirrored ? control.width - width - control.rightPadding : control.leftPadding) : control.leftPadding + (control.availableWidth - width) / 2
        y: control.topPadding + (control.availableHeight - height) / 2
        visible: control.checkable
        control: control
        checkState: control.checked ? Qt.Checked : Qt.Unchecked
    }
    */

    arrow: MD.Icon {
        x: control.mirrored ? control.padding : control.width - width - control.padding
        y: control.topPadding + (control.availableHeight - height) / 2

        visible: control.subMenu
        size: 24
        name: MD.Token.icon.arrow_right
    }

    contentItem: MD.IconLabel {

        //readonly property real arrowPadding: control.subMenu && control.arrow ? control.arrow.width + control.spacing : 0
        //readonly property real indicatorPadding: control.checkable && control.indicator ? control.indicator.width + control.spacing : 0
        //leftPadding: !control.mirrored ? indicatorPadding : arrowPadding
        //rightPadding: control.mirrored ? indicatorPadding : arrowPadding

        horizontalAlignment: Qt.AlignLeft
        spacing: control.spacing

        text: control.text
        font: control.font
        icon_name: control.icon.name
        icon_size: control.icon.width
        icon_color: control.leadingIconColor
    }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 48
        color: "transparent"

        MD.Ripple {
            width: parent.width
            height: parent.height

            clip: visible
            pressed: control.pressed
            anchor: control
            active: control.down || control.highlighted
            color: control.MD.MatProp.stateLayerColor
        }
    }

    MD.MatProp.elevation: item_state.elevation
    MD.MatProp.textColor: item_state.textColor
    MD.MatProp.supportTextColor: item_state.supportTextColor
    MD.MatProp.backgroundColor: item_state.backgroundColor
    MD.MatProp.stateLayerColor: item_state.stateLayerColor

    property color leadingIconColor: item_state.leadingIconColor
    property color trailingIconColor: item_state.trailingIconColor

    MD.State {
        id: item_state
        item: control

        elevation: MD.Token.elevation.level2
        textColor: item_state.ctx.color.on_surface
        backgroundColor: item_state.ctx.color.surface_container
        supportTextColor: item_state.ctx.color.on_surface_variant
        stateLayerColor: "transparent"
        property color leadingIconColor: item_state.ctx.color.on_surface
        property color trailingIconColor: item_state.ctx.color.on_surface

        states: [
            State {
                name: "Disabled"
                when: !enabled
                PropertyChanges {
                    item_state.elevation: MD.Token.elevation.level0
                    control.contentItem.opacity: 0.38
                }
            },
            State {
                name: "Pressed"
                when: control.down || control.focus
                PropertyChanges {
                    item_state.leadingIconColor: item_state.ctx.color.on_surface_variant
                    item_state.trailingIconColor: item_state.ctx.color.on_surface_variant
                    item_state.stateLayerColor: {
                        const c = item_state.ctx.color.on_surface;
                        return MD.Util.transparent(c, MD.Token.state.pressed.state_layer_opacity);
                    }
                }
            },
            State {
                name: "Hovered"
                when:  control.hovered
                PropertyChanges {
                    item_state.leadingIconColor: item_state.ctx.color.on_surface_variant
                    item_state.trailingIconColor: item_state.ctx.color.on_surface_variant
                    item_state.stateLayerColor: {
                        const c = item_state.ctx.color.on_surface;
                        return MD.Util.transparent(c, MD.Token.state.hover.state_layer_opacity);
                    }
                }
            }
        ]
    }
}
