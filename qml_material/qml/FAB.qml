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

    anchors.right: parent.right
    anchors.bottom: parent.bottom
    anchors.rightMargin: 16
    anchors.bottomMargin: 16

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    padding: 16
    spacing: 0

    icon.width: 24
    icon.height: 24

    font.weight: MD.Token.typescale.label_large.weight
    font.pixelSize: Math.min(icon.width, icon.height)
    font.family: MD.Token.font.icon_round.family

    contentItem: Item {
        implicitWidth: control.icon.width
        implicitHeight: control.icon.height
        Text {
            anchors.centerIn: parent
            font: control.font
            text: control.icon.name
            color: MD.MatProp.textColor
            lineHeight: font.pixelSize
            lineHeightMode: Text.FixedHeight
        }
    } 

    background: Rectangle {
        implicitWidth: 56
        implicitHeight: 56

        radius: 16
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