import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import Qcm.Material as MD

T.Button {
    id: control

    property int type: MD.Enum.CardElevated

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    verticalPadding: 0
    horizontalPadding: 16

    contentItem: Item {}

    background: Rectangle {
        implicitWidth: 64
        implicitHeight: 64

        radius: 12
        color: MD.MatProp.backgroundColor

        border.width: control.type == MD.Enum.CardOutlined ? 1 : 0
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
            active: enabled && (control.pressed || control.visualFocus || control.hovered)
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
                    MD.MatProp.elevation: {
                        switch (control.type) {
                        case MD.Enum.CardOutlined:
                        case MD.Enum.CardFilled:
                            return MD.Token.elevation.level0;
                        case MD.Enum.CardElevated:
                        default:
                            return MD.Token.elevation.level1;
                        }
                    }
                    MD.MatProp.backgroundColor: {
                        switch (control.type) {
                        case MD.Enum.CardOutlined:
                            return MD.Token.color.surface;
                        case MD.Enum.CardFilled:
                            return MD.Token.color.surface_container_highest;
                        case MD.Enum.CardElevated:
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
                    MD.MatProp.elevation: {
                        switch (control.type) {
                        case MD.Enum.CardFilled:
                            return MD.Token.elevation.level1;
                        case MD.Enum.CardOutlined:
                        case MD.Enum.CardElevated:
                        default:
                            return MD.Token.elevation.level0;
                        }
                    }
                    MD.MatProp.backgroundColor: MD.Token.color.surface_variant
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
                    MD.MatProp.elevation: {
                        switch (control.type) {
                        case MD.Enum.CardOutlined:
                        case MD.Enum.CardFilled:
                            return MD.Token.elevation.level1;
                        case MD.Enum.CardElevated:
                        default:
                            return MD.Token.elevation.level2;
                        }
                    }
                    MD.MatProp.stateLayerColor: {
                        let c = MD.Token.color.on_surface;
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
                    MD.MatProp.elevation: {
                        switch (control.type) {
                        case MD.Enum.CardOutlined:
                        case MD.Enum.CardFilled:
                            return MD.Token.elevation.level0;
                        case MD.Enum.CardElevated:
                        default:
                            return MD.Token.elevation.level1;
                        }
                    }
                    MD.MatProp.stateLayerColor: {
                        let c = MD.Token.color.on_surface;
                        return Qt.rgba(c.r, c.g, c.b, 0.18);
                    }
                    target: control
                }
            }
        ]
    }
}
