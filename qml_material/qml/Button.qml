import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import Qcm.Material as MD

T.Button {
    id: control

    property int type: MD.Enum.BtElevated
    property int iconStyle: hasIcon ? MD.Enum.IconAndText : MD.Enum.TextOnly
    readonly property bool hasIcon: MD.Util.hasIcon(icon)

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: type == MD.Enum.BtText || type == MD.Enum.BtOutlined
    topInset: 6
    bottomInset: 6
    verticalPadding: 14
    // https://m3.material.io/components/buttons/specs#256326ad-f934-40e7-b05f-0bcb41aa4382
    leftPadding: flat ? 12 : (hasIcon ? 16 : 24)
    rightPadding: flat ? (hasIcon ? 16 : 12) : 24
    spacing: 8

    icon.width: 24
    icon.height: 24

    font.weight: MD.Token.typescale.label_large.weight
    font.pointSize: MD.Token.typescale.label_large.size
    property int lineHeight: MD.Token.typescale.label_large.line_height

    contentItem: MD.IconLabel {
        lineHeight: control.lineHeight

        font: control.font
        text: control.text
        icon_style: control.iconStyle

        icon_name: control.icon.name
        icon_size: control.icon.width
    }

    background: Rectangle {
        implicitWidth: 64
        implicitHeight: 40

        radius: height / 2
        color: control.MD.MatProp.backgroundColor

        border.width: control.type == MD.Enum.BtOutlined ? 1 : 0
        border.color: control.MD.MatProp.color.outline

        // The layer is disabled when the button color is transparent so you can do
        // Material.background: "transparent" and get a proper flat button without needing
        // to set Material.elevation as well
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
        textColor: {
            switch (control.type) {
            case MD.Enum.BtFilled:
            case MD.Enum.BtFilledTonal:
                return ctx.color.getOn(control.MD.MatProp.backgroundColor);
            case MD.Enum.BtOutlined:
            case MD.Enum.BtText:
            case MD.Enum.BtElevated:
            default:
                return ctx.color.primary;
            }
        }
        backgroundColor: {
            switch (control.type) {
            case MD.Enum.BtFilled:
                return ctx.color.primary;
            case MD.Enum.BtFilledTonal:
                return ctx.color.secondary_container;
            case MD.Enum.BtOutlined:
            case MD.Enum.BtText:
                return ctx.color.surface;
            case MD.Enum.BtElevated:
            default:
                return ctx.color.surface_container_low;
            }
        }
        stateLayerColor: "transparent"

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
                when: control.down
                PropertyChanges {
                    item_state.elevation: MD.Token.elevation.level1
                }
                PropertyChanges {
                    restoreEntryValues: false
                    item_state.stateLayerColor: {
                        let c = null;
                        switch (control.type) {
                        case MD.Enum.BtFilled:
                        case MD.Enum.BtFilledTonal:
                            c = item_state.ctx.color.getOn(item_state.ctx.backgroundColor);
                            break;
                        case MD.Enum.BtOutlined:
                        case MD.Enum.BtText:
                        case MD.Enum.BtElevated:
                        default:
                            c = item_state.ctx.color.primary;
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
                        case MD.Enum.BtFilled:
                        case MD.Enum.BtFilledTonal:
                            c = item_state.ctx.color.getOn(item_state.ctx.backgroundColor);
                            break;
                        case MD.Enum.BtOutlined:
                        case MD.Enum.BtText:
                        case MD.Enum.BtElevated:
                        default:
                            c = item_state.ctx.color.primary;
                        }
                        return MD.Util.transparent(c, MD.Token.state.hover.state_layer_opacity);
                    }
                }
            },
            State {
                name: "Focus"
                when: control.focus
                PropertyChanges {
                    item_state.elevation: MD.Token.elevation.level1
                }
                PropertyChanges {
                    restoreEntryValues: false
                    item_state.stateLayerColor: {
                        let c = null;
                        switch (control.type) {
                        case MD.Enum.BtFilled:
                        case MD.Enum.BtFilledTonal:
                            c = item_state.ctx.color.getOn(item_state.ctx.backgroundColor);
                            break;
                        case MD.Enum.BtOutlined:
                        case MD.Enum.BtText:
                        case MD.Enum.BtElevated:
                        default:
                            c = item_state.ctx.color.primary;
                        }
                        return MD.Util.transparent(c, MD.Token.state.focus.state_layer_opacity);
                    }
                }
            }
        ]
    }
}
