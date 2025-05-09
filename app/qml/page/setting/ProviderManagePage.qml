import QtCore
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    font.capitalization: Font.Capitalize
    title: qsTr('provider manage')
    bottomPadding: radius
    scrolling: !m_view.atYBeginning

    MD.VerticalListView {
        id: m_view
        anchors.fill: parent
        leftMargin: 0
        rightMargin: 0
        expand: true
        model: QA.App.providerStatus

        delegate: MD.ListItem {
            required property int index
            required property var model

            width: parent.width
            leader: MD.IconSvg {
                sourceData: QA.App.providerStatus.svg(index)
                size: 24
            }
            text: model.name

            onClicked: {
                root.MD.MProp.page.pushItem('qrc:/Qcm/App/qml/page/edit/ProviderEditPage.qml', {
                    itemId: model.itemId
                });
            }
        }

        footer: ColumnLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 12
            anchors.rightMargin: 12

            MD.Space {
                spacing: 12
            }
            MD.Button {
                Layout.alignment: Qt.AlignRight
                type: MD.Enum.BtElevated
                action: MD.Action {
                    text: qsTr('add')
                    onTriggered: {
                        root.MD.MProp.page.pushItem('qrc:/Qcm/App/qml/page/AddProviderPage.qml');
                    }
                }
            }
            MD.Space {
                spacing: 12
            }
        }
    }
}
