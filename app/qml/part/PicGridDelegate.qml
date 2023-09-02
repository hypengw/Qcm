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

            MD.Text {
                id: label
                Layout.fillWidth: true
                text: root.text
                maximumLineCount: 2
                typescale: MD.Token.typescale.body_medium
            }

            MD.Text {
                id: label_sub
                Layout.alignment: Qt.AlignHCenter
                visible: !!text
                opacity: 0.6
            }
        }
    }
}
