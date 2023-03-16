import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Window
import Qt.labs.settings
import QcmApp

ApplicationWindow {
    id: win

    // load QA
    readonly property string _QA: QA.user_info.nickname

    function push_page(url, props) {
        win_stack.currentItem.page_stack.push_page(url, props);
    }

    width: 700
    height: 450
    visible: true
    Material.primary: Theme.color.primary
    Material.accent: Material.primary
    Material.background: Theme.color.background
    Material.foreground: Theme.color.on_background
    Material.theme: Theme.is_dark_theme ? Material.Dark : Material.Light
    color: Material.background
    Component.onCompleted: {
        QA.main_win = win;
    }

    Settings {
        property alias x: win.x
        property alias y: win.y
        property alias width: win.width
        property alias height: win.height

        category: 'window'
    }

    StackView {
        id: win_stack

        anchors.fill: parent

        Connections {
            function onStatusChanged() {
                if (target.status !== ApiQuerierBase.Finished)
                    return ;

                win_stack.replace(win_stack.currentItem, QA.is_login ? main_view : 'qrc:/QcmApp/qml/page/LoginPage.qml');
            }

            target: QA.querier_user
        }

        initialItem: Item {
        }

    }

    Component {
        id: main_view

        RowLayout {
            readonly property alias page_stack: m_page_stack

            spacing: 0

            Pane {
                z: 1
                Layout.fillHeight: true
                Material.elevation: 2
                Material.background: Theme.color.surface_2
                padding: 0

                ColumnLayout {
                    anchors.fill: parent

                    StackLayout {
                        Layout.fillHeight: false
                        currentIndex: 1

                        ColumnLayout {
                            RoundButton {
                                font.family: Theme.font.icon_round.family
                                font.pointSize: 16
                                flat: true
                                text: Theme.ic.arrow_back
                                onClicked: {
                                    if (m_page_stack.depth > 1)
                                        m_page_stack.pop_page();
                                    else if (page_container.currentItem.canBack)
                                        page_container.currentItem.back();
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
                            Component.onCompleted: {
                                [{
                                    "icon": Theme.ic.menu,
                                    "action": {
                                        "do": function() {
                                        }
                                    }
                                }, {
                                    "icon": Theme.ic.library_music,
                                    "page": 'qrc:/QcmApp/qml/page/MinePage.qml',
                                    "cache": true
                                }, {
                                    "icon": Theme.ic.today,
                                    "page": 'qrc:/QcmApp/qml/page/TodayPage.qml'
                                }].forEach((m) => {
                                    model.append(m);
                                });
                                currentIndex = 1;
                            }
                            onCurrentIndexChanged: {
                                const m = model.get(currentIndex);
                                page_container.switchTo(m.page, m.props, m.cache);
                            }

                            model: ListModel {
                            }

                            delegate: MItemDelegate {
                                width: ListView.view.width
                                Material.primary: Theme.color.tertiary
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

                        }

                        Binding on currentIndex {
                            when: m_page_stack.depth > 1 || !!page_container.currentItem.canBack
                            value: 0
                        }

                    }

                    Item {
                        Layout.fillHeight: true
                    }

                    RoundButton {
                        font.family: Theme.font.icon_round.family
                        font.pointSize: 16
                        flat: true
                        text: Theme.is_dark_theme ? Theme.ic.dark_mode : Theme.ic.light_mode
                        onClicked: {
                            Theme.theme = Theme.is_dark_theme ? MdColorMgr.Light : MdColorMgr.Dark;
                        }
                    }

                    RoundButton {
                        font.family: Theme.font.icon_round.family
                        font.pointSize: 16
                        flat: true
                        text: Theme.ic.settings
                        onClicked: {
                            QA.show_popup('qrc:/QcmApp/qml/part/SettingsPopup.qml');
                        }
                    }

                }

            }

            ColumnLayout {
                spacing: 0

                // clip: true
                PageStack {
                    id: m_page_stack

                    Layout.fillWidth: true
                    Layout.fillHeight: true

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

}
