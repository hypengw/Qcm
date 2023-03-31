import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."
import "../component"
import "../part"

MPage {
    id: root

    property alias text: label.text

    padding: 0
    title: qsTr('description')

    MFlickable {
        id: flick

        anchors.fill: parent

        ColumnLayout {
            id: content

            height: implicitHeight
            width: parent.width
            spacing: 12

            Label {
                id: label

                Layout.fillWidth: true
                wrapMode: Text.Wrap
                elide: Text.ElideRight
                textFormat: Text.PlainText
            }

        }

        ScrollBar.vertical: ScrollBar {
        }

    }

}
