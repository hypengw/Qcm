import QtQuick
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    showHeader: false
    showBackground: true
    MD.MProp.backgroundColor: MD.MProp.color.surface

    Item {
        anchors.fill: parent
        implicitWidth: m_stack.implicitWidth
        implicitHeight: m_stack.implicitHeight

        QA.PageStack {
            id: m_stack
            anchors.fill: parent

            MD.MProp.page: m_page_ctx
            MD.PageContext {
                id: m_page_ctx
                inherit: root.MD.MProp.page
                showHeader: m_stack.depth > 1
                leadingAction: MD.Action {
                    icon.name: MD.Token.icon.arrow_back
                    onTriggered: {
                        m_stack.back();
                    }
                }
            }

            initialItem: MD.Page {
                MD.MProp.page: root.MD.MProp.page
                Item {
                    anchors.fill: parent
                    MD.Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: m_services_layout.top
                        anchors.bottomMargin: 24

                        text: qsTr('add music provider')
                        font.capitalization: Font.Capitalize
                        typescale: MD.Token.typescale.title_large
                    }

                    ColumnLayout {
                        id: m_services_layout
                        anchors.centerIn: parent
                        spacing: 12

                        QA.ProviderMetasQuery {
                            id: m_meta_query
                            Component.onCompleted: reload()
                        }

                        MD.VerticalListView {
                            id: m_view
                            model: m_meta_query.data.metasData
                            expand: true
                            interactive: false
                            implicitWidth: Math.min(400, m_stack.width)
                            leftMargin: 16
                            rightMargin: 16

                            delegate: MD.ListItem {
                                required property int index
                                required property var model
                                width: ListView.view.contentWidth

                                corners: indexCorners(index, count, 16)
                                leader: MD.IconSvg {
                                    sourceData: model.svg
                                    size: 24
                                }
                                action: MD.Action {
                                    text: model.typeName
                                    onTriggered: {
                                        m_stack.push_page("qrc:/Qcm/App/qml/page/ProviderMetaPage.qml", {
                                            meta: model
                                        });
                                    }
                                }
                                mdState: MD.StateListItem {
                                    item: parent
                                    backgroundColor: ctx.color.surface_container
                                }
                                divider: MD.Divider {
                                    anchors.bottom: parent.bottom
                                    orientation: Qt.Horizontal
                                }
                            }
                        }
                    }
                }
            }
        }

        MD.IconButton {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.leftMargin: 4
            anchors.bottomMargin: 4

            action: QA.SettingAction {}
        }
    }
}
