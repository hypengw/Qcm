import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Material as MD

import "../js/util.mjs" as Util

MD.Page {
    id: root
    padding: 0

    Connections {
        function onSig_route(msg) {
            m_page_stack.push_page(msg.qml, msg.props);
        }

        target: QA.Global
    }
    RowLayout {
        anchors.fill: parent
        spacing: 0

        MD.Pane {
            Layout.fillHeight: true
            MD.MatProp.elevation: MD.Token.elevation.level2
            // Material.background: Theme.color.surface_container
            padding: 0
            z: 1

            ColumnLayout {
                anchors.fill: parent

                StackLayout {
                    Layout.fillHeight: false
                    currentIndex: 1

                    Binding on currentIndex  {
                        value: 0
                        when: m_page_stack.depth > 1 || !!page_container.currentItem.canBack
                    }

                    ColumnLayout {
                        MD.IconButton {
                            action: Action {
                                icon.name: MD.Token.icon.arrow_back

                                onTriggered: {
                                    if (m_page_stack.depth > 1)
                                        m_page_stack.pop_page();
                                    else if (page_container.currentItem.canBack)
                                        page_container.currentItem.back();
                                }
                            }
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                    }
                    ListView {
                        Layout.fillWidth: true
                        implicitHeight: contentHeight
                        interactive: false

                        delegate: QA.MItemDelegate {
                            // Material.primary: Theme.color.secondary_container
                            width: ListView.view.width

                            onClicked: {
                                if (model.action)
                                    model.action.do();
                                else
                                    ListView.view.currentIndex = index;
                            }

                            MD.Icon {
                                anchors.centerIn: parent
                                name: model.icon
                                size: 24
                            }
                        }
                        model: ListModel {
                        }

                        Component.onCompleted: {
                            [{
                                    "icon": MD.Token.icon.menu,
                                    "action": {
                                        "do": function () {
                                            App.test();
                                        }
                                    }
                                }, {
                                    "icon": MD.Token.icon.library_music,
                                    "page": 'qrc:/Qcm/App/qml/page/MinePage.qml',
                                    "cache": true
                                }, {
                                    "icon": MD.Token.icon.today,
                                    "page": 'qrc:/Qcm/App/qml/page/TodayPage.qml',
                                    "cache": true
                                }, {
                                    "icon": MD.Token.icon.queue_music,
                                    "page": 'qrc:/Qcm/App/qml/page/PlaylistListPage.qml'
                                }].forEach(m => {
                                    model.append(m);
                                });
                            currentIndex = 1;
                        }
                        onCurrentIndexChanged: {
                            const m = model.get(currentIndex);
                            if (m.page)
                                page_container.switchTo(m.page, m.props, m.cache);
                        }
                    }
                }
                Item {
                    Layout.fillHeight: true
                }
                MD.IconButton {
                    action: Action {
                        readonly property bool is_dark_theme: MD.Token.color.schemeTheme == MD.MdColorMgr.Dark
                        icon.name: is_dark_theme ? MD.Token.icon.dark_mode : MD.Token.icon.light_mode

                        onTriggered: {
                            MD.Token.color.schemeTheme = is_dark_theme ? MD.MdColorMgr.Light : MD.MdColorMgr.Dark;
                        }
                    }
                }
                MD.IconButton {
                    action: Action {
                        icon.name: MD.Token.icon.settings

                        onTriggered: {
                            QA.Global.show_page_popup('qrc:/Qcm/App/qml/page/SettingsPage.qml', {});
                        }
                    }
                }
            }
        }
        ColumnLayout {
            spacing: 0

            // clip: true
            QA.PageStack {
                id: m_page_stack
                Layout.fillHeight: true
                Layout.fillWidth: true
                //Material.background: Theme.color.surface

                initialItem: QA.PageContainer {
                    id: page_container
                    initialItem: Item {
                    }
                }
            }
            MD.Divider {}
            QA.PlayBar {
                Layout.fillWidth: true
                // Material.background: Theme.color.surface_container
            }
        }
    }
}
