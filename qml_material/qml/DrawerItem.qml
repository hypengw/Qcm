import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import QtQuick.Controls.impl
import Qcm.Material as MD

T.ItemDelegate {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)

    topInset: 0
    bottomInset: 0
    leftInset: 0
    rightInset: 0

    verticalPadding: 0
    leftPadding: 16
    rightPadding: 24
    spacing: 0
    checked: false

    icon.width: 24
    icon.height: 24

    property alias trailing: item_holder_trailing.contentItem

    contentItem: RowLayout {
        spacing: 12

        MD.Icon {
            id: item_holder_leader
            name: control.icon.name
            size: Math.min(control.icon.width, control.icon.height)
        }
        MD.Text {
            Layout.fillWidth: true
            font: control.font
            text: control.text
            typescale: MD.Token.typescale.label_large
            maximumLineCount: control.maximumLineCount
            verticalAlignment: Qt.AlignVCenter
        }
        MD.Control {
            id: item_holder_trailing
            Layout.alignment: Qt.AlignVCenter
            visible: contentItem
        }
    }

    background: Rectangle {
        implicitWidth: 336
        implicitHeight: 56

        radius: 28
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

        elevation: MD.Token.elevation.level0
        textColor: control.checked ? on_secondary_container : MD.Token.color.on_surface_variant
        backgroundColor: control.checked ? MD.Token.color.secondary_container : "transparent"
        supportTextColor: MD.Token.color.on_surface_variant
        stateLayerColor: "transparent"

        states: [
            State {
                name: "Disabled"
                when: !control.enabled
                PropertyChanges {
                    item_state.elevation: MD.Token.elevation.level0
                    item_state.textColor: MD.Token.color.on_surface
                    item_state.supportTextColor: MD.Token.color.on_surface
                    item_state.backgroundColor: MD.Token.color.on_surface
                    control.contentItem.opacity: 0.38
                    control.background.opacity: 0.38
                }
            },
            State {
                name: "Hovered"
                when: control.enabled && control.hovered && !control.down
                PropertyChanges {
                    item_state.textColor: control.checked ? on_secondary_container : MD.Token.color.on_surface
                }
                PropertyChanges {
                    restoreEntryValues: false
                    item_state.stateLayerColor: {
                        const c = control.checked ? MD.Token.color.on_secondary_container : MD.Token.color.on_surface;
                        return MD.Util.transparent(c, MD.Token.state.hover.state_layer_opacity);
                    }
                }
            },
            State {
                name: "Pressed"
                when: control.enabled && control.down
                PropertyChanges {
                    item_state.textColor: control.checked ? on_secondary_container : MD.Token.color.on_surface
                }
                PropertyChanges {
                    restoreEntryValues: false
                    item_state.stateLayerColor: {
                        const c = MD.Token.color.on_secondary_container;
                        return MD.Util.transparent(c, MD.Token.state.pressed.state_layer_opacity);
                    }
                }
            }
        ]
    }
}
