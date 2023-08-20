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
        color: MD.MatProp.backgroundColor

        border.width: control.type == MD.Enum.BtOutlined ? 1 : 0
        border.color: MD.Token.color.outline

        // The layer is disabled when the button color is transparent so you can do
        // Material.background: "transparent" and get a proper flat button without needing
        // to set Material.elevation as well
        layer.enabled: control.enabled && color.a > 0 && !control.flat
        layer.effect: MD.RoundedElevationEffect {
            elevation: MD.MatProp.elevation
            roundedScale: control.background.radius
        }

        MD.Ripple {
            clip: true
            clipRadius: parent.radius
            width: parent.width
            height: parent.height
            pressed: control.pressed
            anchor: control
            active: enabled && (control.down || control.visualFocus || control.hovered)
            color: MD.MatProp.stateLayerColor
        }
    }

    Item {
        id: item_btn_state

        visible: false
        state: "Base"
        states: [
            State {
                name: "Base"
                when: control.enabled && !control.hovered && !control.down
                PropertyChanges {
                    MD.MatProp.elevation: MD.Token.elevation.level1
                    MD.MatProp.textColor: {
                        switch (control.type) {
                        case MD.Enum.BtFilled:
                        case MD.Enum.BtFilledTonal:
                            return MD.Token.color.getOn(control.MD.MatProp.backgroundColor);
                        case MD.Enum.BtOutlined:
                        case MD.Enum.BtText:
                        case MD.Enum.BtElevated:
                        default:
                            return MD.Token.color.primary;
                        }
                    }
                    MD.MatProp.backgroundColor: {
                        switch (control.type) {
                        case MD.Enum.BtFilled:
                            return MD.Token.color.primary;
                        case MD.Enum.BtFilledTonal:
                            return MD.Token.color.secondary_container;
                        case MD.Enum.BtOutlined:
                        case MD.Enum.BtText:
                            return MD.Token.color.surface;
                        case MD.Enum.BtElevated:
                        default:
                            return MD.Token.color.surface_container_low;
                        }
                    }
                    MD.MatProp.stateLayerColor: "#00000000"
                    target: control
                    restoreEntryValues: false
                }
            },
            State {
                name: "Disabled"
                when: !enabled
                PropertyChanges {
                    MD.MatProp.elevation: MD.Token.elevation.level0
                    MD.MatProp.textColor: MD.Token.color.on_surface
                    MD.MatProp.backgroundColor: MD.Token.color.on_surface
                    contentItem.opacity: 0.38
                    background.opacity: 0.12
                    target: control
                }
            },
            State {
                name: "Hovered"
                when: control.enabled && control.hovered && !control.down
                PropertyChanges {
                    MD.MatProp.elevation: MD.Token.elevation.level2
                    MD.MatProp.stateLayerColor: {
                        let c = null;
                        switch (control.type) {
                        case MD.Enum.BtFilled:
                        case MD.Enum.BtFilledTonal:
                            c = MD.Token.color.getOn(control.MD.MatProp.backgroundColor);
                            break;
                        case MD.Enum.BtOutlined:
                        case MD.Enum.BtText:
                        case MD.Enum.BtElevated:
                        default:
                            c = MD.Token.color.primary;
                        }
                        return Qt.rgba(c.r, c.g, c.b, 0.08);
                    }
                    target: control
                }
            },
            State {
                name: "Pressed"
                when: control.enabled && control.down
                PropertyChanges {
                    MD.MatProp.elevation: MD.Token.elevation.level1
                    MD.MatProp.stateLayerColor: {
                        let c = null;
                        switch (control.type) {
                        case MD.Enum.BtFilled:
                        case MD.Enum.BtFilledTonal:
                            c = MD.Token.color.getOn(control.MD.MatProp.backgroundColor);
                            break;
                        case MD.Enum.BtOutlined:
                        case MD.Enum.BtText:
                        case MD.Enum.BtElevated:
                        default:
                            c = MD.Token.color.primary;
                        }
                        return Qt.rgba(c.r, c.g, c.b, 0.18);
                    }
                    target: control
                }
            }
        ]
    }
}
