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
    property string _QA: QA.user_info.nickname

    function route(dest) {
        let url = dest;
        let props = {
        };
        switch (dest.type) {
        case ItemId.Album:
            url = 'qrc:/QcmApp/qml/page/AlbumDetailPage.qml';
            props = {
                "itemId": dest
            };
            break;
        case ItemId.Playlist:
            url = 'qrc:/QcmApp/qml/page/PlaylistDetailPage.qml';
            props = {
                "itemId": dest
            };
            break;
        case ItemId.Artist:
            url = 'qrc:/QcmApp/qml/page/ArtistDetailPage.qml';
            props = {
                "itemId": dest
            };
            break;
        }
        win_stack.currentItem.page_stack?.push(url, props);
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
        QA.main_win = win
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
                                    if (page_container.currentItem.canBack)
                                        page_container.currentItem.back();
                                    else
                                        m_page_stack.pop();
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                        }

                        ListView {
                            Layout.fillWidth: true
                            implicitHeight: contentHeight
                            currentIndex: 1
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
                            }

                            model: ListModel {
                            }

                            delegate: MItemDelegate {
                                width: ListView.view.width
                                Material.primary: Theme.color.tertiary
                                onClicked: {
                                    if (model.action) {
                                        model.action.do();
                                    } else {
                                        ListView.view.currentIndex = index;
                                        page_container.switchTo(model.page, model.props, model.cache);
                                    }
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
                    }

                }

            }

            ColumnLayout {
                spacing: 0

                // clip: true
                StackView {
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
