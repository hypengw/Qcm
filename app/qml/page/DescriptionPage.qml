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

            anchors.fill: parent
            spacing: 12

            Label {
                id: label

                Layout.fillWidth: true
                wrapMode: Text.Wrap
                elide: Text.ElideRight
                textFormat: Text.PlainText
                font.pointSize: Theme.ts.label_medium.size
            }

        }

        ScrollBar.vertical: ScrollBar {
        }

    }

}
