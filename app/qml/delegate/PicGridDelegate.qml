import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Material as MD

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
                text: root.subText
                visible: !!text
                opacity: 0.6
                typescale: MD.Token.typescale.body_medium
            }
        }
    }
}
