import QtQuick
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    Item {
        id: root_item
        anchors.fill: parent
        implicitHeight: 2 * (24 + m_text.implicitHeight + 24 + m_services_layout.implicitHeight / 2)
        implicitWidth: Math.max(m_text.implicitWidth, m_services_layout.implicitWidth) + 16 * 2

        MD.Text {
            id: m_text
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: m_services_layout.top
            anchors.bottomMargin: 24

            text: qsTr('add music provider')
            font.capitalization: Font.Capitalize
            typescale: MD.Token.typescale.title_large
        }

        ColumnLayout {
            id: m_services_layout
            y: root.height > root_item.implicitHeight ? (root.height - height) / 2 : m_text.height + 24 * 2
            height: root.height > root_item.implicitHeight ? implicitHeight : root.height - (m_text.height + 24 * 2)
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 12

            QA.ProviderMetasQuery {
                id: m_meta_query
                Component.onCompleted: reload()
            }

            MD.VerticalListView {
                id: m_view
                Layout.fillHeight: true
                model: m_meta_query.data.metasData
                expand: true
                implicitWidth: Math.min(400, root.width)
                leftMargin: 16
                rightMargin: 16

                delegate: MD.ListItem {
                    width: ListView.view.contentWidth

                    corners: MD.Util.listCorners(index, count, 16)
                    leader: MD.IconSvg {
                        sourceData: model.svg
                        size: 24
                    }
                    action: MD.Action {
                        text: model.typeName
                        onTriggered: {
                            root.MD.MProp.page.pushItem("qrc:/Qcm/App/qml/page/ProviderMetaPage.qml", {
                                meta: model
                            });
                        }
                    }
                    mdState.backgroundColor: mdState.ctx.color.surface_container
                    divider: MD.Divider {
                        anchors.bottom: parent.bottom
                        orientation: Qt.Horizontal
                    }
                }
            }
        }
    }
}
