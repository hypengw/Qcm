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
    readonly property alias snake: m_snake

    Material.accent: Material.primary
    Material.background: Theme.color.background
    Material.foreground: Theme.color.on_background
    Material.primary: Theme.color.primary
    Material.theme: Theme.toMatTheme(Theme.theme)
    color: Material.background
    height: 600
    visible: true
    width: 900

    Component.onCompleted: {
        QA.main_win = win;
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
        StackLayout {
            id: sl_main
            clip: true

            Connections {
                function onSig_route_special(name) {
                    if (name === 'main')
                        sl_main.currentIndex = 0;
                    else if (name === 'playing')
                        sl_main.currentIndex = 1;
                }

                target: QA
            }
            MainPage {
            }
            PlayingPage {
                Layout.fillWidth: true
            }
        }
    }
    Component {
        id: comp_login
        LoginPage {
        }
    }
}
