import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    title: qsTr('status')
    padding: 0
    bottomPadding: radius
    scrolling: !m_flick.atYBeginning

    MD.VerticalFlickable {
        id: m_flick
        anchors.fill: parent
        topMargin: 4
        leftMargin: 24
        rightMargin: 24
        bottomMargin: 4

        ColumnLayout {
            id: content

            height: implicitHeight
            width: parent.width
            spacing: 8

            MD.Text {
                Layout.fillWidth: true
                typescale: MD.Token.typescale.body_large
                text: QA.App.import_path_list().join('\n')
                maximumLineCount: 12
            }
        }
    }
}