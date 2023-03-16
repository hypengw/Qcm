import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

RowLayout {
    id: root

    property bool firstline: false
    property int lineHeight: -1
    property alias text: icon.text
    property alias iconSize: icon.font.pointSize
    property alias color: icon.color

    Label {
        id: icon

        Layout.alignment: root.firstline ? Qt.AlignTop : Qt.AlignVCenter
        Layout.preferredHeight: root.lineHeight
        font.family: Theme.font.icon_round.family
        verticalAlignment: Qt.AlignVCenter
    }

}
