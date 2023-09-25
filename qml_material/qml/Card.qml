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

    MD.MatProp.elevation: item_state.elevation
    MD.MatProp.textColor: item_state.textColor
    MD.MatProp.supportTextColor: item_state.supportTextColor
    MD.MatProp.backgroundColor: item_state.backgroundColor
    MD.MatProp.stateLayerColor: item_state.stateLayerColor

    MD.State {
        id: item_state
        visible: false

        elevation: {
            switch (control.type) {
            case MD.Enum.CardOutlined:
            case MD.Enum.CardFilled:
                return MD.Token.elevation.level0;
            case MD.Enum.CardElevated:
            default:
                return MD.Token.elevation.level1;
            }
        }
        textColor: MD.Token.color.getOn(backgroundColor)
        backgroundColor: {
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
        stateLayerColor: "#00000000"
        states: [
            State {
                name: "Disabled"
                when: !enabled
                PropertyChanges {
                    item_state.elevation: {
                        switch (control.type) {
                        case MD.Enum.CardFilled:
                            return MD.Token.elevation.level1;
                        case MD.Enum.CardOutlined:
                        case MD.Enum.CardElevated:
                        default:
                            return MD.Token.elevation.level0;
                        }
                    }
                    item_state.backgroundColor: MD.Token.color.surface_variant
                    control.contentItem.opacity: 0.38
                    control.background.opacity: 0.12
                }
            },
            State {
                name: "Hovered"
                when: control.enabled && control.hovered && !control.down
                PropertyChanges {
                    item_state.elevation: {
                        switch (control.type) {
                        case MD.Enum.CardOutlined:
                        case MD.Enum.CardFilled:
                            return MD.Token.elevation.level1;
                        case MD.Enum.CardElevated:
                        default:
                            return MD.Token.elevation.level2;
                        }
                    }
                    item_state.stateLayerColor: {
                        let c = MD.Token.color.on_surface;
                        return MD.Util.transparent(c, MD.Token.state.hover.state_layer_opacity);
                    }
                }
            },
            State {
                name: "Pressed"
                when: control.enabled && control.down
                PropertyChanges {
                    item_state.elevation: {
                        switch (control.type) {
                        case MD.Enum.CardOutlined:
                        case MD.Enum.CardFilled:
                            return MD.Token.elevation.level0;
                        case MD.Enum.CardElevated:
                        default:
                            return MD.Token.elevation.level1;
                        }
                    }
                    item_state.stateLayerColor: {
                        let c = MD.Token.color.on_surface;
                        return MD.Util.transparent(c, MD.Token.state.pressed.state_layer_opacity);
                    }
                }
            }
        ]
    }
}
