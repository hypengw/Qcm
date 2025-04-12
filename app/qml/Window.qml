import QtCore
import QtQuick
import QtQml
import QtQuick.Window
import QtQuick.Templates as T

import Qcm.App as QA
import Qcm.Material as MD

MD.ApplicationWindow {
    id: win

    // load QA
    readonly property alias snake: m_snake
    property int windowClass: MD.Enum.WindowClassMedium

    MD.MProp.page.leadingAction: MD.Action {
        icon.name: MD.Token.icon.menu
        text: qsTr('menu')
        onTriggered: {
            QA.Action.open_drawer();
        }
    }

    MD.MProp.size.windowClass: windowClass
    MD.MProp.backgroundColor: {
        switch (win.windowClass) {
        case MD.Enum.WindowClassCompact:
            return MD.Token.color.surface;
        default:
            return MD.Token.color.surface_container;
        }
    }
    MD.MProp.textColor: MD.MProp.color.getOn(MD.MProp.backgroundColor)

    color: MD.MProp.backgroundColor
    height: 600
    visible: true
    width: 900

    onWidthChanged: {
        windowClass = MD.Token.window_class.select_type(width);
    }

    function back() {
        if (m_win_stack.currentItem.canBack) {
            m_win_stack.currentItem.back();
        } else {
            console.log("back to quit");
        }
    }

    QA.PageStack {
        id: m_win_stack
        anchors.fill: parent

        initialItem: comp_loading

        MD.MProp.page: m_stack_page_ctx
        MD.PageContext {
            id: m_stack_page_ctx
            radius: win.MD.MProp.size.isCompact ? 0 : MD.Token.shape.corner.large
            showHeader: m_win_stack.depth > 1
            leadingAction: MD.Action {
                icon.name: MD.Token.icon.arrow_back
                onTriggered: {
                    m_win_stack.pop();
                }
            }
        }

        Component.onCompleted: QA.App.appState.onQmlCompleted()

        Connections {
            function onLoading() {
                m_win_stack.pop(null);
                m_win_stack.replace(m_win_stack.currentItem, comp_loading);
            }
            function onWelcome() {
                m_win_stack.pop(null);
                m_win_stack.replace(m_win_stack.currentItem, comp_login);
            }
            function onMain() {
                m_win_stack.pop(null);
                m_win_stack.replace(m_win_stack.currentItem, comp_main);
            }
            function onError(err) {
                console.error(err);
                m_win_stack.pop(null);
                m_win_stack.replace(m_win_stack.currentItem, comp_retry, {
                    text: err,
                    retryCallback: () => {
                        QA.App.appState.retry();
                    }
                });
            }
            target: QA.App.appState
        }
        Connections {
            function onRoute_special(name) {
                if (name === QA.Enum.SRLogin) {
                    m_win_stack.push(comp_login);
                }
            }

            target: QA.Action
        }
    }

    MD.SnakeView {
        id: m_snake
        parent: T.Overlay.overlay
        anchors.fill: parent
        MD.InputBlock {
            target: m_snake
        }
    }

    QA.PagePopup {
        id: m_sync_popup
        source: 'qrc:/Qcm/App/qml/page/SyncPage.qml'
    }

    QA.PagePopup {
        id: m_queue_popup
        source: 'qrc:/Qcm/App/qml/page/PlayQueuePage.qml'
    }

    Connections {
        target: QA.Action
        function onPopup_special(s) {
            if (s === QA.Enum.SRQueue) {
                m_queue_popup.open();
            } else if (s === QA.Enum.SRSync) {
                m_sync_popup.open();
            } else if (typeof s == 'number') {
                const url = QA.Util.special_route_url(s);
                if (url)
                    QA.Action.popup_page(url, {});
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
            win.snake.show(text, duration, flags, action);
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
        QA.LoadingPage {}
    }
}
