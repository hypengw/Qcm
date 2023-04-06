import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qt.labs.settings
import QcmApp
import ".."
import "../component"
import "../part"
import "../js/util.mjs" as Util

Page {
    id: root
    padding: 0

    Connections {
        function onSig_route(msg) {
            m_page_stack.push_page(msg.qml, msg.props);
        }

        target: QA
    }
    RowLayout {
        anchors.fill: parent
        spacing: 0

        Pane {
            Layout.fillHeight: true
            Material.background: Theme.color.surface_2
            Material.elevation: 2
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
                        MRoundButton {
                            flat: true

                            action: Action {
                                icon.name: Theme.ic.arrow_back

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

                        delegate: MItemDelegate {
                            Material.primary: Theme.color.tertiary
                            width: ListView.view.width

                            onClicked: {
                                if (model.action)
                                    model.action.do();
                                else
                                    ListView.view.currentIndex = index;
                            }

                            Label {
                                anchors.centerIn: parent
                                font.family: Theme.font.icon_round.family
                                font.pointSize: 16
                                text: model.icon
                            }
                        }
                        model: ListModel {
                        }

                        Component.onCompleted: {
                            [{
                                    "icon": Theme.ic.menu,
                                    "action": {
                                        "do": function () {
                                            App.test();
                                        }
                                    }
                                }, {
                                    "icon": Theme.ic.library_music,
                                    "page": 'qrc:/QcmApp/qml/page/MinePage.qml',
                                    "cache": true
                                }, {
                                    "icon": Theme.ic.today,
                                    "page": 'qrc:/QcmApp/qml/page/TodayPage.qml',
                                    "cache": true
                                }, {
                                    "icon": Theme.ic.queue_music,
                                    "page": 'qrc:/QcmApp/qml/page/PlaylistListPage.qml'
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
                MRoundButton {
                    flat: true

                    action: Action {
                        icon.name: Theme.is_dark_theme ? Theme.ic.dark_mode : Theme.ic.light_mode

                        onTriggered: {
                            Theme.theme = Theme.is_dark_theme ? MdColorMgr.Light : MdColorMgr.Dark;
                        }
                    }
                }
                MRoundButton {
                    flat: true

                    action: Action {
                        icon.name: Theme.ic.settings

                        onTriggered: {
                            QA.show_page_popup('qrc:/QcmApp/qml/page/SettingsPage.qml', {});
                        }
                    }
                }
            }
        }
        ColumnLayout {
            spacing: 0

            // clip: true
            PageStack {
                id: m_page_stack
                Layout.fillHeight: true
                Layout.fillWidth: true

                initialItem: PageContainer {
                    id: page_container
                    initialItem: Item {
                    }
                }
            }
            PlayBar {
                Layout.fillWidth: true
            }
        }
    }
}
