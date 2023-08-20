import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Controls.impl
import Qcm.Material as MD

T.ItemDelegate {
    id: control

    property int iconStyle: hasIcon ? MD.Enum.IconAndText : MD.Enum.TextOnly
    readonly property bool hasIcon: MD.Util.hasIcon(icon)

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    topInset: 6
    bottomInset: 6
    verticalPadding: 14
    leftPadding: 0
    rightPadding: 0
    spacing: 8

    icon.width: 24
    icon.height: 24

    font.weight: MD.Token.typescale.label_large.weight
    font.pointSize: MD.Token.typescale.label_large.size
    property int lineHeight: MD.Token.typescale.label_large.line_height

    contentItem: RowLayout {
        ColumnLayout {
            Text {
                id: item_text_head
            }
            Text {
                id: item_text_support
            }
        }
    }

    background: Rectangle {
        implicitWidth: 64
        implicitHeight: 40

        radius: height / 2
        color: MD.MatProp.backgroundColor

        border.width: control.type == MD.Enum.BtOutlined ? 1 : 0
        border.color: MD.Token.color.outline

        layer.enabled: control.enabled && color.a > 0
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
