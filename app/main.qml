import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Qcm.App as QA
import Qcm.Material as MD

ApplicationWindow {
    id: win

    // load QA
    readonly property string _QA: QA.Global.user_info.nickname
    readonly property alias snake: m_snake

    MD.MatProp.backgroundColor: MD.MatProp.color.background
    MD.MatProp.textColor: MD.MatProp.color.getOn(MD.MatProp.backgroundColor)

    color: MD.MatProp.backgroundColor
    height: 600
    visible: true
    width: 900

    Component.onCompleted: {
        QA.Global.main_win = win;
    }

    Connections {
        function onInstanceStarted() {
            win.raise();
            win.requestActivate();
        }

        target: QA.App
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
                running: QA.Global.querier_user.status === QA.qcm.Querying
            }
        }

        Connections {
            function onStatusChanged() {
                if (target.status === QA.qcm.Error) {
                    win_stack.replace(win_stack.currentItem, comp_retry, {
                            text: target.error,
                            retryCallback: () => {
                                target.query();
                            }
                        });
                } else if (target.status === QA.qcm.Finished) {
                    win_stack.replace(win_stack.currentItem, QA.Global.is_login ? comp_main : comp_login);
                }
            }

            target: QA.Global.querier_user
        }
    }
    QA.SnakeView {
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
                const page = QA.Global.create_item(comp_playing, {}, sv_main);
                page.visible = false;
                playing_page = page;
            }

            clip: true

            initialItem: QA.MainPage {
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

                target: QA.Global
            }
            Component {
                id: comp_playing
                QA.PlayingPage {
                }
            }
        }
    }
    Component {
        id: comp_login
        QA.LoginPage {
        }
    }
    Component {
        id: comp_retry
        QA.RetryPage {
        }
    }
}
