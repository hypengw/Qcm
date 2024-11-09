import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    property alias text: label.text
    title: qsTr('description')
    bottomPadding: radius
    scrolling: !m_flick.atYBeginning

    MD.Flickable {
        id: m_flick

        anchors.fill: parent
        leftMargin: 24
        rightMargin: 24

        ColumnLayout {
            id: content

            height: implicitHeight
            width: parent.width
            spacing: 12

            MD.TextEdit {
                id: label
                Layout.fillWidth: true
                readOnly: true
                wrapMode: Text.Wrap
                typescale: MD.Token.typescale.body_medium
            }
        }
    }
}
