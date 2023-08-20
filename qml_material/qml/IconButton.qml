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
        implicitWidth: 40
        implicitHeight: 40

        radius: height / 2
        color: MD.MatProp.backgroundColor

        border.width: control.type == MD.Enum.IBtOutlined ? 1 : 0
        border.color: MD.Token.color.outline

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

        state: "Base"
        states: [
            State {
                name: "Base"
                when: control.enabled && !control.hovered && !control.down
                PropertyChanges {
                    MD.MatProp.elevation: MD.Token.elevation.level1
                    MD.MatProp.textColor: {
                        switch (control.type) {
                        case MD.Enum.iBtOutlined:
                            if(control.checked)
                                return MD.Token.color.on_inverse_surface;
                            else
                                return MD.Token.color.on_surface_variant;
                        case MD.Enum.IBtStandard:
                            if(control.checked)
                                return MD.Token.color.primary;
                            else
                                return MD.Token.color.on_surface_variant;
                        case MD.Enum.IBtFilledTonal:
                            if(!control.checkable || control.checked)
                                return MD.Token.color.on_secondary_container;
                            else
                                return MD.Token.color.on_surface_variant;
                        case MD.Enum.IBtFilled:
                        default:
                            if(!control.checkable || control.checked)
                                return MD.Token.color.on_primary;
                            else
                                return MD.Token.color.primary;
                        }
                    }
                    MD.MatProp.backgroundColor: {
                        switch (control.type) {
                        case MD.Enum.IBtStandard:
                                return "#00000000";
                        case MD.Enum.IBtOutlined:
                            if(control.checked)
                                return MD.Token.color.inverse_surface2;
                            else
                                return "#00000000";
                        case MD.Enum.IBtFilledTonal:
                            if(!control.checkable || control.checked)
                                return MD.Token.color.secondary_container;
                            else
                                return MD.Token.color.surface_container_highest;
                        case MD.Enum.IBtFilled:
                        default:
                            if(!control.checkable || control.checked)
                                return MD.Token.color.primary;
                            else
                                return MD.Token.color.surface_container_highest;
                        }
                    }
                    MD.MatProp.stateLayerColor: "#00000000"
                    restoreEntryValues: false
                    target: control
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
                extend: "Base"
                when: control.enabled && control.hovered && !control.down
                PropertyChanges {
                    MD.MatProp.elevation: MD.Token.elevation.level2
                    MD.MatProp.stateLayerColor: {
                        let c = null;
                        switch (control.type) {
                        case MD.Enum.IBtFilled:
                            if(!control.checkable || control.checked)
                                c = MD.Token.color.on_primary;
                            else
                                c = MD.Token.color.primary;
                            break;
                        case MD.Enum.IBtFilledTonal:
                            if(!control.checkable || control.checked)
                                c = MD.Token.color.on_secondary_container;
                            else
                                c = MD.Token.color.on_surface_variant;
                            break;
                        case MD.Enum.IBtOutlined:
                            if(control.checked)
                                c = MD.Token.color.on_inverse_surface;
                            else
                                c = MD.Token.color.on_surface_variant;
                            break;
                        default:
                        case MD.Enum.IBtStandard:
                            if(control.checked)
                                c = MD.Token.color.primary;
                            else
                                c = MD.Token.color.on_surface_variant;
                            break;
                        }
                        return Qt.rgba(c.r, c.g, c.b, 0.08);
                    }
                    target: control
                }
            },
            State {
                name: "Pressed"
                extend: "Base"
                when: control.enabled && control.down
                PropertyChanges {
                    MD.MatProp.elevation: MD.Token.elevation.level1
                    MD.MatProp.stateLayerColor: {
                        let c = null;
                        switch (control.type) {
                        case MD.Enum.IBtFilled:
                            if(!control.checkable || control.checked)
                                c = MD.Token.color.on_primary;
                            else
                                c = MD.Token.color.primary;
                            break;
                        case MD.Enum.IBtFilledTonal:
                            if(!control.checkable || control.checked)
                                c = MD.Token.color.on_secondary_container;
                            else
                                c = MD.Token.color.on_surface_variant;
                            break;
                        case MD.Enum.IBtOutlined:
                            if(control.checked)
                                c = MD.Token.color.on_inverse_surface;
                            else
                                c = MD.Token.color.on_surface;
                            break;
                        case MD.Enum.IBtStandard:
                        default:
                            if(control.checked)
                                c = MD.Token.color.primary;
                            else
                                c = MD.Token.color.on_surface_variant;
                        }
                        return Qt.rgba(c.r, c.g, c.b, 0.18);
                    }
                    target: control
                }
            }
        ]
    }
}
