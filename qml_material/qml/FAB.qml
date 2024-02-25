import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import Qcm.Material as MD

T.Button {
    id: control

    property int type: MD.Enum.FABNormal
    property int color: MD.Enum.FABColorPrimary

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: false

    anchors.right: parent.right
    anchors.bottom: parent.bottom
    anchors.rightMargin: 16
    anchors.bottomMargin: 16

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    padding: _size(type, 8, 16, 30)
    spacing: 0

    icon.width: _size(type, 24, 24, 36)
    icon.height: _size(type, 24, 24, 36)

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
        implicitWidth: _size(type, 40, 56, 96)
        implicitHeight: _size(type, 40, 56, 96)

        radius: _size(type, 12, 16, 28)
        color: MD.MatProp.backgroundColor

        border.width: control.type == MD.Enum.BtOutlined ? 1 : 0
        border.color: item_state.ctx.color.outline

        // The layer is disabled when the button color is transparent so you can do
        // Material.background: "transparent" and get a proper flat button without needing
        // to set Material.elevation as well
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
            active: enabled && (control.down || control.visualFocus || control.hovered)
            color: MD.MatProp.stateLayerColor
        }
    }

    function _size(t, small, normal, large) {
        return t == MD.Enum.FABSmall ? small : (t == MD.Enum.FABLarge ? large : normal);
    }

    MD.MatProp.elevation: item_state.elevation
    MD.MatProp.textColor: item_state.textColor
    MD.MatProp.supportTextColor: item_state.supportTextColor
    MD.MatProp.backgroundColor: item_state.backgroundColor
    MD.MatProp.stateLayerColor: item_state.stateLayerColor

    MD.State {
        id: item_state
        item: control

        elevation: MD.Token.elevation.level3
        textColor: {
            switch (control.color) {
            case MD.Enum.FABColorSurfaec:
                return item_state.ctx.color.primary;
            case MD.Enum.FABColorSecondary:
                return item_state.ctx.color.on_secondary_container;
            case MD.Enum.FABColorTertiary:
                return item_state.ctx.color.on_tertiary_container;
            case MD.Enum.FABColorPrimary:
            default:
                return item_state.ctx.color.on_primary_container;
            }
        }
        backgroundColor: {
            switch (control.color) {
            case MD.Enum.FABColorSurfaec:
                return item_state.ctx.color.surface_container_high;
            case MD.Enum.FABColorSecondary:
                return item_state.ctx.color.secondary_container;
            case MD.Enum.FABColorTertiary:
                return item_state.ctx.color.tertiary_container;
            case MD.Enum.FABColorPrimary:
            default:
                return item_state.ctx.color.primary_container;
            }
        }
        stateLayerColor: "transparent"

        states: [
            State {
                name: "Pressed"
                when: control.down || control.focus
                PropertyChanges {
                    item_state.elevation: MD.Token.elevation.level3
                }
                PropertyChanges {
                    restoreEntryValues: false
                    item_state.stateLayerColor: {
                        const c = item_state.textColor;
                        return MD.Util.transparent(c, MD.Token.state.pressed.state_layer_opacity);
                    }
                }
            },
            State {
                name: "Hovered"
                when: control.hovered 
                PropertyChanges {
                    item_state.elevation: MD.Token.elevation.level4
                }
                PropertyChanges {
                    restoreEntryValues: false
                    item_state.stateLayerColor: {
                        const c = item_state.textColor;
                        return MD.Util.transparent(c, MD.Token.state.hover.state_layer_opacity);
                    }
                }
            }
        ]
    }
}
