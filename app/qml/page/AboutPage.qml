import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    title: qsTr('about')
    padding: 0
    readonly property var info: QA.App.info()

    MD.Flickable {
        id: flick
        anchors.fill: parent
        leftMargin: 24
        rightMargin: 24

        ColumnLayout {
            id: content

            height: implicitHeight
            width: parent.width
            spacing: 8

            MD.Image {
                property int size: 96
                Layout.preferredHeight: size
                Layout.preferredWidth: size
                Layout.alignment: Qt.AlignHCenter
                source: 'qrc:/Qcm/App/assets/Qcm.svg'
            }

            MD.Text {
                Layout.alignment: Qt.AlignHCenter
                typescale: MD.Token.typescale.headline_small
                text: root.info.name
            }
            MD.Text {
                Layout.alignment: Qt.AlignHCenter
                typescale: MD.Token.typescale.label_medium
                opacity: 0.8
                text: root.info.version
            }
            MD.Text {
                Layout.alignment: Qt.AlignHCenter
                typescale: MD.Token.typescale.label_large
                text: root.info.author
            }

            MD.Text {
                Layout.topMargin: 8
                Layout.bottomMargin: 8
                Layout.alignment: Qt.AlignHCenter
                typescale: MD.Token.typescale.body_large
                text: root.info.summary
            }

            MD.DrawerItem {
                Layout.fillWidth: true
                icon.name: MD.Token.icon['public']
                text: qsTr('project website')
                font.capitalization: Font.Capitalize
                onClicked: {
                    Qt.openUrlExternally('https://github.com/hypengw/Qcm');
                }
            }
            MD.DrawerItem {
                Layout.fillWidth: true
                icon.name: MD.Token.icon.flag
                font.capitalization: Font.Capitalize
                text: qsTr('report an issue')
                onClicked: {
                    Qt.openUrlExternally('https://github.com/hypengw/Qcm/issues');
                }
            }
        }
    }
}
