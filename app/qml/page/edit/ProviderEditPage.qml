import QtCore
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root

    required property QA.item_id itemId
    readonly property var model: QA.App.providerStatus.itemById(itemId)

    font.capitalization: Font.Capitalize
    title: qsTr('provider edit')
    bottomPadding: radius
    scrolling: !m_flick.atYBeginning

    MD.VerticalFlickable {
        id: m_flick
        anchors.fill: parent
        bottomMargin: 24
        contentHeight: m_main.implicitHeight

        ColumnLayout {
            id: m_main
            anchors.centerIn: parent
            Layout.preferredWidth: 300
            spacing: 12

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                MD.IconSvg {
                    Layout.alignment: Qt.AlignHCenter
                    sourceData: QA.App.providerStatus.svg(root.itemId)
                    size: 42
                }
                MD.Label {
                    typescale: MD.Token.typescale.title_large
                    text: root.model.typeName
                }
            }

            MD.TextField {
                id: m_tf_name
                type: MD.Enum.TextFieldFilled
                Layout.minimumWidth: 300
                Layout.fillWidth: true
                placeholderText: qsTr('Provider Name')
                text: root.model.name
            }
        }
    }
}
