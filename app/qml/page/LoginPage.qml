import QtQuick
import QtQuick.Controls as QC
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    showHeader: false

    Item {
        anchors.fill: parent
        implicitWidth: m_stack.implicitWidth
        implicitHeight: m_stack.implicitHeight

        QA.PageStack {
            id: m_stack
            anchors.fill: parent

            property QC.Action barAction: QC.Action {
                icon.name: MD.Token.icon.arrow_back
                onTriggered: {
                    m_stack.back();
                }
            }

            initialItem: MD.Page {
                pageContext: root.pageContext
                Item {
                    anchors.fill: parent
                    MD.Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: m_services_layout.top
                        anchors.bottomMargin: 24

                        text: qsTr('add music service')
                        font.capitalization: Font.Capitalize
                        typescale: MD.Token.typescale.title_large
                    }

                    ColumnLayout {
                        id: m_services_layout
                        anchors.centerIn: parent

                        QA.PluginModel {
                            id: m_plugin_model
                        }

                        MD.ListView {
                            id: m_view
                            model: m_plugin_model
                            expand: true
                            implicitWidth: Math.min(400, m_stack.width)
                            leftMargin: 16
                            rightMargin: 16

                            delegate: MD.ListItem {
                                required property int index
                                required property var model
                                width: ListView.view.contentWidth

                                radius: indexRadius(index, count, 16)
                                leader: MD.IconSvg {
                                    source: model.info.icon
                                    size: 24
                                }
                                action: QC.Action {
                                    text: model.info.fullname
                                    onTriggered: {
                                        const url = model.router.basic_page(QA.enums.BPageLogin);
                                        m_stack.push_page(model.router.route_url(url));
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
            anchors.leftMargin: 16
            anchors.bottomMargin: 16

            action: QA.ColorSchemeAction {}
        }
    }
}
