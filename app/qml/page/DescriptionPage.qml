import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Material as MD


MD.Page {
    id: root

    property alias text: label.text

    horizontalPadding: 24
    title: qsTr('description')

    QA.MFlickable {
        id: flick

        anchors.fill: parent

        ColumnLayout {
            id: content

            height: implicitHeight
            width: parent.width
            spacing: 12

            MD.Text {
                id: label
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                maximumLineCount: -1
                typescale: MD.Token.typescale.body_medium
            }

        }

        ScrollBar.vertical: ScrollBar {
        }

    }

}
