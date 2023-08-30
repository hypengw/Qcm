import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qt.labs.settings
import Qcm.App
import ".."
import "../component"
import "../part"
import "../js/util.mjs" as Util
import Qcm.Material as MD

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
            Material.background: Theme.color.surface_container
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
                            Material.primary: Theme.color.secondary_container
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
                                    "page": 'qrc:/Qcm/App/qml/page/MinePage.qml',
                                    "cache": true
                                }, {
                                    "icon": Theme.ic.today,
                                    "page": 'qrc:/Qcm/App/qml/page/TodayPage.qml',
                                    "cache": true
                                }, {
                                    "icon": Theme.ic.queue_music,
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
                        icon.name: is_dark_theme ? Theme.ic.dark_mode : Theme.ic.light_mode

                        onTriggered: {
                            MD.Token.color.schemeTheme = is_dark_theme ? MD.MdColorMgr.Light : MD.MdColorMgr.Dark;
                        }
                    }
                }
                MD.IconButton {
                    action: Action {
                        icon.name: Theme.ic.settings

                        onTriggered: {
                            QA.show_page_popup('qrc:/Qcm/App/qml/page/SettingsPage.qml', {});
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
                Material.background: Theme.color.surface

                initialItem: PageContainer {
                    id: page_container
                    initialItem: Item {
                    }
                }
            }
            PlayBar {
                Layout.fillWidth: true
                Material.background: Theme.color.surface_container
            }
        }
    }
}
