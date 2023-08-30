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
            Text {
                Layout.fillWidth: true
                text: control.text
                color: MD.MatProp.textColor
                font.pixelSize: MD.Token.typescale.body_large.size
                font.weight: MD.Token.typescale.body_large.weight
                lineHeight: MD.Token.typescale.body_large.line_height
                lineHeightMode: Text.FixedHeight
                wrapMode: Text.Wrap
                elide: Text.ElideRight
                maximumLineCount: control.maximumLineCount
                textFormat: Text.PlainText
            }
            Text {
                Layout.fillWidth: true
                text: control.supportText
                color: MD.MatProp.supportTextColor
                font.pixelSize: MD.Token.typescale.body_medium.size
                font.weight: MD.Token.typescale.body_medium.weight
                lineHeight: MD.Token.typescale.body_medium.line_height
                lineHeightMode: Text.FixedHeight
                elide: Text.ElideRight
                textFormat: Text.PlainText
            }
        }
        Text {
            id: item_text_trailing_support
            Layout.alignment: Qt.AlignVCenter
            visible: text.length
            color: MD.MatProp.supportTextColor
            font.pointSize: MD.Token.typescale.label_small.size
            font.weight: MD.Token.typescale.label_small.weight
            lineHeight: MD.Token.typescale.label_small.line_height
            lineHeightMode: Text.FixedHeight
        }

        MD.Icon {
            Layout.alignment: Qt.AlignVCenter
            id: item_text_trailing_icon
            visible: name.length
            size: 24
        }
    }

    background: Rectangle {
        implicitWidth: 64
        implicitHeight: {
            if(control.supportText.length) {
                return 72
            }
            return 56;
        }

        radius: 0
        color: MD.MatProp.backgroundColor

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
        visible: false
        state: "Base"
        states: [
            State {
                name: "Base"
                when: control.enabled && !control.hovered && !control.down
                PropertyChanges {
                    MD.MatProp.elevation: MD.Token.elevation.level0
                    MD.MatProp.textColor: MD.Token.color.on_surface
                    MD.MatProp.supportTextColor: MD.Token.color.on_surface_variant
                    MD.MatProp.backgroundColor: MD.Token.color.surface
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
                    MD.MatProp.supportTextColor: MD.Token.color.on_surface
                    MD.MatProp.backgroundColor: MD.Token.color.on_surface
                    contentItem.opacity: 0.38
                    background.opacity: 0.38
                    target: control
                }
            },
            State {
                name: "Hovered"
                when: control.enabled && control.hovered && !control.down
                PropertyChanges {
                    MD.MatProp.stateLayerColor: MD.Util.hoverColor(MD.Token.color.on_surface)
                    target: control
                }
            },
            State {
                name: "Pressed"
                when: control.enabled && control.down
                PropertyChanges {
                    MD.MatProp.stateLayerColor: MD.Util.pressColor(MD.Token.color.on_surface)
                    target: control
                }
            }
        ]
    }
}
