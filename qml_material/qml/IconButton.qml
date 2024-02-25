import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import Qcm.Material as MD

T.Button {
    id: control

    property int type: MD.Enum.IBtStandard

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: type == MD.Enum.IBtStandard || (type == MD.Enum.IBtOutlined && !control.checked)
    topInset: 4
    bottomInset: 4
    leftInset: 4
    rightInset: 4

    padding: 8
    spacing: 0

    icon.width: 24
    icon.height: 24

    contentItem: Item {
        implicitWidth: control.icon.width
        implicitHeight: control.icon.height
        MD.Icon {
            anchors.centerIn: parent
            name: icon.name
            size: Math.min(icon.width, icon.height)
        }
    }

    background: Rectangle {
        implicitWidth: 40
        implicitHeight: 40

        radius: height / 2
        color: control.MD.MatProp.backgroundColor

        border.width: control.type == MD.Enum.IBtOutlined ? 1 : 0
        border.color: item_state.ctx.color.outline

        layer.enabled: control.enabled && color.a > 0 && !control.flat
        layer.effect: MD.RoundedElevationEffect {
            elevation: control.MD.MatProp.elevation
        }

        MD.Ripple {
            clip: true
            clipRadius: parent.radius
            width: parent.width
            height: parent.height
            pressed: control.pressed
            anchor: control
            active: enabled && (control.down || control.visualFocus || control.hovered)
            color: control.MD.MatProp.stateLayerColor
        }
    }

    MD.MatProp.elevation: item_state.elevation
    MD.MatProp.textColor: item_state.textColor
    MD.MatProp.supportTextColor: item_state.supportTextColor
    MD.MatProp.backgroundColor: item_state.backgroundColor
    MD.MatProp.stateLayerColor: item_state.stateLayerColor

    MD.State {
        id: item_state
        item: control

        elevation: MD.Token.elevation.level1
        stateLayerColor: "transparent"

        textColor: {
            switch (control.type) {
            case MD.Enum.iBtOutlined:
                if (control.checked)
                    return item_state.ctx.color.on_inverse_surface;
                else
                    return item_state.ctx.color.on_surface_variant;
            case MD.Enum.IBtStandard:
                if (control.checked)
                    return item_state.ctx.color.primary;
                else
                    return item_state.ctx.color.on_surface_variant;
            case MD.Enum.IBtFilledTonal:
                if (!control.checkable || control.checked)
                    return item_state.ctx.color.on_secondary_container;
                else
                    return item_state.ctx.color.on_surface_variant;
            case MD.Enum.IBtFilled:
            default:
                if (!control.checkable || control.checked)
                    return item_state.ctx.color.on_primary;
                else
                    return item_state.ctx.color.primary;
            }
        }
        backgroundColor: {
            switch (control.type) {
            case MD.Enum.IBtStandard:
                return "transparent";
            case MD.Enum.IBtOutlined:
                if (control.checked)
                    return item_state.ctx.color.inverse_surface2;
                else
                    return "transparent";
            case MD.Enum.IBtFilledTonal:
                if (!control.checkable || control.checked)
                    return item_state.ctx.color.secondary_container;
                else
                    return item_state.ctx.color.surface_container_highest;
            case MD.Enum.IBtFilled:
            default:
                if (!control.checkable || control.checked)
                    return item_state.ctx.color.primary;
                else
                    return item_state.ctx.color.surface_container_highest;
            }
        }
        states: [
            State {
                name: "Disabled"
                when: !enabled
                PropertyChanges {
                    item_state.elevation: MD.Token.elevation.level0
                    item_state.textColor: item_state.ctx.color.on_surface
                    item_state.backgroundColor: item_state.ctx.color.on_surface
                    control.contentItem.opacity: 0.38
                    control.background.opacity: 0.12
                }
            },
            State {
                name: "Pressed"
                when: control.down || control.focus
                PropertyChanges {
                    item_state.elevation: MD.Token.elevation.level1
                }
                PropertyChanges {
                    restoreEntryValues: false
                    item_state.stateLayerColor: {
                        let c = null;
                        switch (control.type) {
                        case MD.Enum.IBtFilled:
                            if (!control.checkable || control.checked)
                                c = item_state.ctx.color.on_primary;
                            else
                                c = item_state.ctx.color.primary;
                            break;
                        case MD.Enum.IBtFilledTonal:
                            if (!control.checkable || control.checked)
                                c = item_state.ctx.color.on_secondary_container;
                            else
                                c = item_state.ctx.color.on_surface_variant;
                            break;
                        case MD.Enum.IBtOutlined:
                            if (control.checked)
                                c = item_state.ctx.color.on_inverse_surface;
                            else
                                c = item_state.ctx.color.on_surface;
                            break;
                        case MD.Enum.IBtStandard:
                        default:
                            if (control.checked)
                                c = item_state.ctx.color.primary;
                            else
                                c = item_state.ctx.color.on_surface_variant;
                        }
                        return MD.Util.transparent(c, MD.Token.state.pressed.state_layer_opacity);
                    }
                }
            },
            State {
                name: "Hovered"
                when: control.hovered
                PropertyChanges {
                    item_state.elevation: MD.Token.elevation.level2
                }
                PropertyChanges {
                    restoreEntryValues: false
                    item_state.stateLayerColor: {
                        let c = null;
                        switch (control.type) {
                        case MD.Enum.IBtFilled:
                            if (!control.checkable || control.checked)
                                c = item_state.ctx.color.on_primary;
                            else
                                c = item_state.ctx.color.primary;
                            break;
                        case MD.Enum.IBtFilledTonal:
                            if (!control.checkable || control.checked)
                                c = item_state.ctx.color.on_secondary_container;
                            else
                                c = item_state.ctx.color.on_surface_variant;
                            break;
                        case MD.Enum.IBtOutlined:
                            if (control.checked)
                                c = item_state.ctx.color.on_inverse_surface;
                            else
                                c = item_state.ctx.color.on_surface_variant;
                            break;
                        default:
                        case MD.Enum.IBtStandard:
                            if (control.checked)
                                c = item_state.ctx.color.primary;
                            else
                                c = item_state.ctx.color.on_surface_variant;
                            break;
                        }
                        return MD.Util.transparent(c, MD.Token.state.hover.state_layer_opacity);
                    }
                }
            }
        ]
    }
}
