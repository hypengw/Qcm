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

    verticalPadding: 8
    leftPadding: 16
    rightPadding: 24
    spacing: 0

    icon.width: 24
    icon.height: 24

    property string supportText
    property int maximumLineCount: 1
    property alias leader: item_holder_leader.contentItem
    property alias trailing: item_holder_trailing.contentItem

    property int heightMode: {
        if (supportText)
            return MD.Enum.ListItemTwoLine;
        else
            return MD.Enum.ListItemOneLine;
    }

    contentItem: RowLayout {
        spacing: 16

        MD.Control {
            id: item_holder_leader
            visible: contentItem
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 0
            MD.Text {
                Layout.fillWidth: true
                text: control.text
                typescale: MD.Token.typescale.body_large
                maximumLineCount: control.maximumLineCount
                verticalAlignment: Qt.AlignVCenter
            }
            MD.Text {
                Layout.fillWidth: true
                visible: text
                text: control.supportText
                color: MD.MatProp.supportTextColor
                typescale: MD.Token.typescale.body_medium
                verticalAlignment: Qt.AlignVCenter
            }
        }
        MD.Text {
            id: item_text_trailing_support
            Layout.alignment: Qt.AlignVCenter
            visible: text
            typescale: MD.Token.typescale.label_small
            verticalAlignment: Qt.AlignVCenter
        }

        MD.Control {
            id: item_holder_trailing
            Layout.alignment: Qt.AlignVCenter
            visible: contentItem
        }

        MD.Icon {
            id: item_text_trailing_icon
            Layout.alignment: Qt.AlignVCenter
            visible: name.length
            size: 24
        }
    }

    background: Rectangle {
        implicitWidth: 64
        implicitHeight: {
            switch (control.heightMode) {
            case MD.Enum.ListItemThreeLine:
                return 96;
            case MD.Enum.ListItemTwoLine:
                return 72;
            case MD.Enum.ListItemOneLine:
            default:
                return 56;
            }
        }

        radius: 0
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
        textColor: MD.Token.color.on_surface
        backgroundColor: MD.Token.color.surface
        supportTextColor: MD.Token.color.on_surface_variant
        stateLayerColor: "transparent"

        states: [
            State {
                name: "Disabled"
                when: !enabled
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
