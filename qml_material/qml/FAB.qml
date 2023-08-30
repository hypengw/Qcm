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

    function _size(t, small, normal, large) {
        return t == MD.Enum.FABSmall ? small : (t == MD.Enum.FABLarge ? large : normal);
    }

    Item {
        id: item_btn_state

        visible: false
        state: "Base"
        states: [
            State {
                name: "Base"
                when: !control.hovered && !control.down
                PropertyChanges {
                    MD.MatProp.elevation: MD.Token.elevation.level3
                    MD.MatProp.textColor: {
                        switch (control.color) {
                        case MD.Enum.FABColorSurfaec:
                            return MD.Token.color.primary;
                        case MD.Enum.FABColorSecondary:
                            return MD.Token.color.on_secondary_container;
                        case MD.Enum.FABColorTertiary:
                            return MD.Token.color.on_tertiary_container;
                        case MD.Enum.FABColorPrimary:
                        default:
                            return MD.Token.color.on_primary_container;
                        }
                    }
                    MD.MatProp.backgroundColor: {
                        switch (control.color) {
                        case MD.Enum.FABColorSurfaec:
                            return MD.Token.color.surface_container_high;
                        case MD.Enum.FABColorSecondary:
                            return MD.Token.color.secondary_container;
                        case MD.Enum.FABColorTertiary:
                            return MD.Token.color.tertiary_container;
                        case MD.Enum.FABColorPrimary:
                        default:
                            return MD.Token.color.primary_container;
                        }
                    }
                    MD.MatProp.stateLayerColor: "#00000000"
                    target: control
                    restoreEntryValues: false
                }
            },
            State {
                name: "Hovered"
                extend: "Base"
                when: control.hovered && !control.down
                PropertyChanges {
                    MD.MatProp.elevation: MD.Token.elevation.level4
                    MD.MatProp.stateLayerColor: MD.Util.hoverColor(MD.MatProp.textColor)
                    target: control
                }
            },
            State {
                name: "Pressed"
                extend: "Base"
                when: control.down
                PropertyChanges {
                    MD.MatProp.elevation: MD.Token.elevation.level3
                    MD.MatProp.stateLayerColor: MD.Util.pressColor(MD.MatProp.textColor)
                    target: control
                }
            }
        ]
    }
}
