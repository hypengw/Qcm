import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Window
import QcmApp

ApplicationWindow {
    id: win

    // load QA
    readonly property string _QA: QA.user_info.nickname
    readonly property alias snake: m_snake

    Material.accent: Material.primary
    Material.background: Theme.color.background
    Material.foreground: Theme.color.getOn(Material.background)
    Material.primary: Theme.color.primary
    Material.theme: Theme.toMatTheme(Theme.theme)
    color: Material.background
    height: 600
    visible: true
    width: 900

    Component.onCompleted: {
        QA.main_win = win;
    }

    Connections {
        function onInstanceStarted() {
            win.raise();
            win.requestActivate();
        }

        target: App
    }
    Settings {
        property alias height: win.height
        property alias width: win.width
        property alias x: win.x
        property alias y: win.y

        category: 'window'
    }
    StackView {
        id: win_stack
        anchors.fill: parent

        initialItem: Item {
            BusyIndicator {
                anchors.centerIn: parent
                running: QA.querier_user.status === ApiQuerierBase.Querying
            }
        }

        Connections {
            function onStatusChanged() {
                if (target.status !== ApiQuerierBase.Finished)
                    return;
                win_stack.replace(win_stack.currentItem, QA.is_login ? comp_main : comp_login);
            }

            target: QA.querier_user
        }
    }
    SnakeView {
        /*
        anchors.top: parent.top
        anchors.topMargin: 24
        anchors.horizontalCenter: parent.horizontalCenter
        width: Math.min(Math.max(implicitWidth, 350), parent.width)
        */
        id: m_snake
        anchors.fill: parent
    }
    Component {
        id: comp_main
        StackView {
            id: sv_main

            property var playing_page: {
                const page = QA.create_item(comp_playing, {}, sv_main);
                page.visible = false;
                playing_page = page;
            }

            Material.background: Theme.color.surface
            clip: true

            initialItem: MainPage {
            }

            Component.onDestruction: {
                playing_page.destroy();
            }

            Connections {
                function onSig_route_special(name) {
                    if (name === 'main')
                        sv_main.pop(null);
                    else if (name === 'playing')
                        sv_main.push(sv_main.playing_page);
                }

                target: QA
            }
            Component {
                id: comp_playing
                PlayingPage {
                }
            }
        }
    }
    Component {
        id: comp_login
        LoginPage {
        }
    }
}
