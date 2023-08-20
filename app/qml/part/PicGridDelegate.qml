import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qcm.Material as MD
import ".."
import "../component"
import "../part"

MD.Card {
    id: root

    property alias image: image
    property string subText
    property int picWidth: 160

    horizontalPadding: 0

    contentItem: ColumnLayout {
        MD.Image {
            id: image
            radius: root.background.radius
            implicitWidth: root.picWidth
            implicitHeight: root.picWidth

            sourceSize.width: root.picWidth
            sourceSize.height: root.picWidth
        }

        ColumnLayout {
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.topMargin: 8
            Layout.bottomMargin: 8
            Layout.fillWidth: true

            Label {
                id: label
                Layout.fillWidth: true
                maximumLineCount: 2
                wrapMode: Text.Wrap
                elide: Text.ElideRight

                text: root.text
            }

            Label {
                id: label_sub

                visible: !!text
                Layout.alignment: Qt.AlignHCenter
                font.pointSize: Theme.ts.label_small.size
                opacity: 0.6
            }
        }
    }
}
