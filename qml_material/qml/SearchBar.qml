import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Controls as QC
import QtQuick.Controls.impl
import QtQuick.Controls.Material.impl as MDImpl
import Qcm.Material as MD

T.Button {
    id: control

    property bool leading_input: false
    property QC.Action leading_action: QC.Action {
        icon.name: MD.Token.icon.search
    }
    property bool trailing_input: control.text
    property QC.Action trailing_action: QC.Action {
        icon.name: control.text ? MD.Token.icon.close : null
        onTriggered: {
            item_input.text = '';
        }
    }
    signal accepted

    text: item_input.text

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    padding: 0
    leftPadding: 0
    rightPadding: item_trailing.visible ? 0 : 16

    contentItem: RowLayout {
        spacing: 0

        MD.IconButton {
            id: item_leading
            action: control.leading_action
            MD.InputBlock {
                when: !control.leading_input
                target: item_leading
            }
        }

        MD.TextFieldEmbed {
            id: item_input
            Layout.fillWidth: true
            color: control.MD.MatProp.textColor

            Connections {
                target: item_input
                function onAccepted() {
                    control.accepted();
                }
            }
        }

        MD.IconButton {
            id: item_trailing
            action: control.trailing_action
            visible: icon.name
            MD.InputBlock {
                when: !control.trailing_input
                target: item_trailing
            }
        }
    }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 56

        radius: height / 2
        color: control.MD.MatProp.backgroundColor

        layer.enabled: control.enabled && color.a > 0
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
        visible: false

        elevation: MD.Token.elevation.level2
        textColor: MD.Token.color.on_surface
        backgroundColor: MD.Token.color.surface_container_highest
        supportTextColor: MD.Token.color.on_surface_variant
        stateLayerColor: "transparent"//MD.Token.color.surface_tint

        property color outlineColor: MD.Token.color.outline

        states: [
            State {
                name: "Disabled"
                when: !enabled
                PropertyChanges {
                    item_state.supportTextColor: MD.Token.color.on_surface
                    placeholder.opacity: 0.38
                    control.background.opacity: 0.12
                }
            },
            State {
                name: "Hovered"
                when: control.enabled && control.hovered
                PropertyChanges {
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
                when: control.enabled && !control.hovered && control.pressed
                PropertyChanges {
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
