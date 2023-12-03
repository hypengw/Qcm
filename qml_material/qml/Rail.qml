import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Controls.impl
import Qcm.Material as MD

T.Button {
    id: control

    property int iconStyle: hasIcon ? MD.Enum.IconAndText : MD.Enum.TextOnly
    readonly property bool hasIcon: MD.Util.hasIcon(icon)

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    flat: false
    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    padding: 0
    spacing: 8

    icon.width: 24
    icon.height: 24

    font.weight: MD.Token.typescale.label_large.weight
    font.pointSize: MD.Token.typescale.label_large.size
    property int lineHeight: MD.Token.typescale.label_large.line_height

    contentItem: ColumnLayout {
        spacing: 4
        MD.Control {
            Layout.alignment: Qt.AlignHCenter
            hoverEnabled: false
            focusPolicy: Qt.NoFocus
            leftInset: 12
            rightInset: 12

            contentItem: MD.Icon {
                name: control.icon.name
                size: control.icon.width
                color: control.MD.MatProp.supportTextColor
            }

            background: Item {
                implicitWidth: 56
                implicitHeight: 32
                Rectangle {
                    anchors.centerIn: parent
                    height: parent.height
                    width: 56

                    NumberAnimation on width {
                        alwaysRunToEnd: true
                        from: 48
                        to: 56
                        duration: 100
                        running: control.checked
                    }

                    radius: height / 2
                    color: control.MD.MatProp.backgroundColor

                    layer.enabled: control.enabled && color.a > 0 && !control.flat
                    layer.effect: MD.RoundedElevationEffect {
                        elevation: MD.Token.elevation.level0//control.MD.MatProp.elevation
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
            }
        }

        MD.Text {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            font.capitalization: Font.Capitalize
            typescale: MD.Token.typescale.label_medium
            text: control.text
            prominent: control.checked
        }
    }

    background: Item {
    }

    MD.MatProp.elevation: item_state.elevation
    MD.MatProp.textColor: item_state.textColor
    MD.MatProp.supportTextColor: item_state.supportTextColor // icon color
    MD.MatProp.backgroundColor: item_state.backgroundColor
    MD.MatProp.stateLayerColor: item_state.stateLayerColor

    MD.State {
        id: item_state
        visible: false

        elevation: MD.Token.elevation.level1
        textColor: control.checked ? MD.Token.color.on_surface : MD.Token.color.on_surface_variant
        backgroundColor: control.checked ? MD.Token.color.secondary_container : "transparent"
        supportTextColor: control.checked ? MD.Token.color.on_secondary_container : MD.Token.color.on_surface_variant
        stateLayerColor: "transparent"

        states: [
            State {
                name: "Hovered"
                when: control.enabled && control.hovered && !control.down
                PropertyChanges {
                    item_state.elevation: MD.Token.elevation.level2
                    item_state.textColor: MD.Token.color.on_surface
                    item_state.supportTextColor: control.checked ? MD.Token.color.on_secondary_container : MD.Token.color.on_surface
                }
                PropertyChanges {
                    restoreEntryValues: false
                    item_state.stateLayerColor: {
                        const c = MD.Token.color.on_surface;
                        return MD.Util.transparent(c, MD.Token.state.hover.state_layer_opacity);
                    }
                }
            },
            State {
                name: "Pressed"
                when: control.enabled && control.down
                PropertyChanges {
                    item_state.elevation: MD.Token.elevation.level1
                    item_state.textColor: MD.Token.color.on_surface
                    item_state.supportTextColor: control.checked ? MD.Token.color.on_secondary_container : MD.Token.color.on_surface
                }
                PropertyChanges {
                    restoreEntryValues: false
                    item_state.stateLayerColor: {
                        const c = MD.Token.color.on_surface;
                        return MD.Util.transparent(c, MD.Token.state.pressed.state_layer_opacity);
                    }
                }
            }
        ]
    }
}
