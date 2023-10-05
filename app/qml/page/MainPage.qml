import QtCore
import QtQuick
import QtQuick.Controls
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

        ColumnLayout {
            Layout.topMargin: 12
            Layout.preferredWidth: 56 + 24
            Layout.fillWidth: false

            StackLayout {
                Layout.fillHeight: false
                currentIndex: 1

                Binding on currentIndex  {
                    value: 0
                    when: m_page_stack.depth > 1 || !!page_container.currentItem.canBack
                }

                ColumnLayout {
                    MD.IconButton {
                        Layout.alignment: Qt.AlignHCenter
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
                    spacing: 12

                    delegate: MD.Rail {
                        width: ListView.view.width
                        icon.name: model.icon
                        text: model.name
                        checked: ListView.view.currentIndex == index
                        onClicked: {
                            if (model.action)
                                model.action.do();
                            else
                                ListView.view.currentIndex = index;
                        }
                    }

                    model: ListModel {
                    }

                    Component.onCompleted: {
                        [{
                                "icon": MD.Token.icon.menu,
                                "name": qsTr('menu'),
                                "action": {
                                    "do": function () {}
                                }
                            }, {
                                "icon": MD.Token.icon.library_music,
                                "name": qsTr('library'),
                                "page": 'qrc:/Qcm/App/qml/page/MinePage.qml',
                                "cache": true
                            }, {
                                "icon": MD.Token.icon.today,
                                "name": qsTr('today'),
                                "page": 'qrc:/Qcm/App/qml/page/TodayPage.qml',
                                "cache": true
                            }, {
                                "icon": MD.Token.icon.queue_music,
                                "name": qsTr('playlist'),
                                "page": 'qrc:/Qcm/App/qml/page/PlaylistListPage.qml'
                            }
                            /*
                            {
                                "icon": MD.Token.icon.queue_music,
                                "name": qsTr('test'),
                                "page": 'qrc:/Qcm/App/qml/page/MaterialTest.qml'
                            }
                            */
                            ,].forEach(m => {
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
                Layout.alignment: Qt.AlignHCenter
                action: Action {
                    icon.name: MD.Token.icon.search
                    onTriggered: {
                    }
                }
            }
            MD.IconButton {
                Layout.alignment: Qt.AlignHCenter
                action: Action {
                    readonly property bool is_dark_theme: QA.Global.color_scheme == MD.MdColorMgr.Dark
                    icon.name: is_dark_theme ? MD.Token.icon.dark_mode : MD.Token.icon.light_mode

                    onTriggered: {
                        QA.Global.color_scheme = is_dark_theme ? MD.MdColorMgr.Light : MD.MdColorMgr.Dark;
                    }
                }
            }
            MD.IconButton {
                Layout.alignment: Qt.AlignHCenter
                action: Action {
                    icon.name: MD.Token.icon.settings

                    onTriggered: {
                        QA.Global.show_page_popup('qrc:/Qcm/App/qml/page/SettingsPage.qml', {});
                    }
                }
            }
        }

        ColumnLayout {
            spacing: 0
            QA.PageStack {
                id: m_page_stack
                Layout.fillHeight: true
                Layout.fillWidth: true
                clip: true

                //Material.background: Theme.color.surface

                initialItem: QA.PageContainer {
                    id: page_container
                    initialItem: Item {
                    }
                }
            }
            QA.PlayBar {
                Layout.fillWidth: true
                // Material.background: Theme.color.surface_container
            }
        }
    }
}
