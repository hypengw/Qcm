import QtQuick
import QtQuick.Layouts
import Qcm.Material as MD

Item {
    id: root

    implicitWidth: layout_row.implicitWidth
    implicitHeight: layout_row.implicitHeight

    property alias spacing: layout_row.spacing

    property alias text: item_label_text.text
    property alias font: item_label_text.font

    property alias icon_name: item_label_icon.text
    property alias icon_size: item_label_icon.font.pixelSize

    property int lineHeight: MD.Token.typescale.label_large.line_height
    property int icon_style: MD.Enum.IconAndText

    RowLayout {
        id: layout_row

        anchors.fill: parent
        spacing: 8

        Text {
            id: item_label_icon
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            visible: root.icon_style != MD.Enum.TextOnly && text.length > 0

            font.family: MD.Token.font.icon_round.family
            color: MD.MatProp.textColor

            lineHeight: root.lineHeight
            lineHeightMode: Text.FixedHeight
        }

        Text {
            id: item_label_text
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            visible: root.icon_style != MD.Enum.IconOnly
            color: MD.MatProp.textColor
            lineHeight: root.lineHeight
            lineHeightMode: Text.FixedHeight
        }

    }
}