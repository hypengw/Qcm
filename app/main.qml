import QtCore
import QtQuick
import QtQml
import QtQuick.Controls.Basic
import QtQuick.Window

import Qcm.App as QA
import Qcm.Material as MD

ApplicationWindow {
    id: win

    // load QA
    readonly property alias snake: m_snake
    property int windowClass: MD.Enum.WindowClassMedium

    MD.MatProp.page.leadingAction: Action {
        icon.name: MD.Token.icon.menu
        text: qsTr('menu')
        onTriggered: {
            QA.Action.open_drawer();
        }
    }

    MD.MatProp.size.windowClass: windowClass
    MD.MatProp.backgroundColor: {
        switch (win.windowClass) {
        case MD.Enum.WindowClassCompact:
            return MD.Token.color.surface;
        default:
            return MD.Token.color.surface_container;
        }
    }
    MD.MatProp.textColor: MD.MatProp.color.getOn(MD.MatProp.backgroundColor)

    color: MD.MatProp.backgroundColor
    height: 600
    visible: true
    width: 900

    onWidthChanged: {
        windowClass = MD.Token.window_class.select_type(width);
    }

    function back() {
        if (win_stack.currentItem.canBack) {
            win_stack.currentItem.back();
        } else {
            console.log("back to quit");
        }
    }

    QA.PageStack {
        id: win_stack
        anchors.fill: parent

        initialItem: comp_loading

        MD.MatProp.page: m_stack_page_ctx
        MD.PageContext {
            id: m_stack_page_ctx
            radius: win.MD.MatProp.size.isCompact ? 0 : MD.Token.shape.corner.large
            showHeader: win_stack.depth > 1
            leadingAction: Action {
                icon.name: MD.Token.icon.arrow_back
                onTriggered: {
                    win_stack.pop();
                }
            }
        }

        Connections {
            function onStart() {
                win_stack.pop(null);
                win_stack.replace(win_stack.currentItem, comp_login);
            }
            function onSession() {
                win_stack.pop(null);
                win_stack.replace(win_stack.currentItem, comp_main);
            }
            function onError(err) {
                win_stack.pop(null);
                win_stack.replace(win_stack.currentItem, comp_retry, {
                    text: err,
                    retryCallback: () => {
                        QA.Global.appState.rescue.reload();
                    }
                });
            }
            target: QA.Global.appState
        }
        Connections {
            function onRoute_special(name) {
                if (name === QA.enums.SRLogin) {
                    win_stack.push(comp_login);
                }
            }

            target: QA.Action
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
        parent: Overlay.overlay
        anchors.fill: parent
        MD.InputBlock {
            target: m_snake
        }
    }

    QA.ProviderMetasQuery {
        Component.onCompleted: reload()
    }

    QA.PagePopup {
        id: queue_popup
        source: 'qrc:/Qcm/App/qml/page/PlayQueuePage.qml'

        Connections {
            target: QA.Action
            function onPopup_special(s) {
                if (s === QA.enums.SRQueue) {
                    queue_popup.open();
                } else if (typeof s == 'number') {
                    const url = QA.Util.special_route_url(s);
                    if(url) QA.Action.popup_page(url, {});
                }
            }

            function onPopup_page(url, props, popup_props, callback) {
                const popup = MD.Util.show_popup('qrc:/Qcm/App/qml/component/PagePopup.qml', Object.assign({}, {
                    "source": url,
                    "props": props
                }, popup_props), win);
                if (callback) {
                    callback(popup);
                }
            }
        }
    }

    Timer {
        running: !win.active
        interval: 10 * 1000
        onTriggered: {
            QA.App.releaseResources(win);
        }
    }

    Settings {
        property alias height: win.height
        property alias width: win.width
        property alias x: win.x
        property alias y: win.y

        category: 'window'
    }

    Shortcut {
        sequences: [StandardKey.Back, StandardKey.Cancel, StandardKey.Close]
        onActivated: win.back()
    }
    Shortcut {
        sequence: StandardKey.Quit
        context: Qt.ApplicationShortcut
        onActivated: Qt.quit()
    }

    Connections {
        target: QA.Action
        function onToast(text, duration, flags, action) {
            win.snake.show2(text, duration, flags, action);
        }
    }
    Connections {
        function onInstanceStarted() {
            win.raise();
            win.requestActivate();
        }

        target: QA.App
    }

    Component.onCompleted: {
        QA.Global.main_win = win;
    }

    Component {
        id: comp_main
        MD.StackView {
            id: sv_main

            property var playing_page: {
                const page = MD.Util.create_item(comp_playing, {}, sv_main);
                page.visible = false;
                playing_page = page;
            }

            clip: true

            initialItem: QA.MainPage {}

            property bool canBack: currentItem.canBack || depth > 1
            function back() {
                if (currentItem.canBack) {
                    this.currentItem.back();
                } else {
                    this.pop(null);
                }
            }

            Component.onDestruction: {
                playing_page.destroy();
            }

            Connections {
                function onRoute_special(name) {
                    if (name === 'main')
                        sv_main.pop(null);
                    else if (name === 'playing')
                        sv_main.push(sv_main.playing_page);
                }

                target: QA.Action
            }
            Component {
                id: comp_playing
                QA.PlayingPage {}
            }
        }
    }
    Component {
        id: comp_login
        QA.LoginPage {}
    }
    Component {
        id: comp_retry
        QA.RetryPage {}
    }
    Component {
        id: comp_loading
        Item {
            MD.CircularIndicator {
                anchors.centerIn: parent
                running: visible
            }
        }
    }
}
