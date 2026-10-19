pragma ValueTypeBehavior: Assertable
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

    MD.MProp.page.leadingAction: MD.Action {
        icon.name: MD.Token.icon.menu
        text: qsTr('menu')
        onTriggered: {
            QA.Action.openDrawer();
        }
    }

    MD.MProp.size.width: width
    MD.MProp.backgroundColor: {
        const c = MD.MProp.size.windowClass;
        switch (c) {
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

        initialItem: QA.LoadingPage {}

        MD.MProp.page: m_stack_page_ctx
        MD.PageContext {
            id: m_stack_page_ctx
            radius: win.MD.MProp.size.isCompact ? 0 : MD.Token.shape.corner.large
            showHeader: m_win_stack.depth > 1
            leadingAction: MD.Action {
                icon.name: MD.Token.icon.arrow_back
                onTriggered: {
                    m_win_stack.pop_page();
                }
            }
        }

        Component.onCompleted: QA.App.appState.onQmlCompleted()

        Connections {
            function onLoading() {
                m_win_stack.pop_page(null);
                m_win_stack.replace_page(comp_loading);
            }
            function onWelcome() {
                m_win_stack.pop_page(null);
                m_win_stack.replace_page(comp_welcome);
            }
            function onMain() {
                m_win_stack.pop_page(null);
                m_win_stack.replace_page(comp_main);
            }
            function onError(err) {
                console.error(err);
                m_win_stack.pop_page(null);
                m_win_stack.replace_page(comp_retry, {
                    text: err,
                    retryCallback: () => {
                        QA.App.appState.retry();
                    }
                });
            }
            target: QA.App.appState
        }
        Connections {
            function onRoute(name) {
                if (name === QA.Enum.SRLogin) {
                    m_win_stack.push_page(comp_welcome);
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
        function onOpenPopup(s) {
            if (s === QA.Enum.SRQueue) {
                m_queue_popup.open();
            } else if (s === QA.Enum.SRSync) {
                m_sync_popup.open();
            } else if (typeof s == 'number') {
                const purl = 'Qcm.App/PagePopup';
                const url = QA.Util.special_route_url(s);
                if (url) {
                    const props = {source: url};
                    if (s === QA.Enum.SRSetting)
                        props.implicitWidth = 420;
                    MD.Util.showPopup(purl, props, win);
                }
            } else if (typeof s == 'string') {
                MD.Util.showPopup(s, {}, win);
            } else if (s as QA.rmsg) {
                const msg = s as QA.rmsg;
                MD.Util.showPopup(msg.dst, msg.props, win);
            } else if (s as Component) {
                MD.Util.showPopup(s, {}, win);
            } else {
                console.error(s);
            }
        }

        function onOpenItemMenu(id, parent, props) {
            let url = "";
            switch (id.type) {
            case QA.Enum.ItemAlbum:
                url = 'Qcm.App/AlbumMenu';
                break;
            case QA.Enum.ItemArtist:
            case QA.Enum.ItemAlbumArtist:
                url = 'Qcm.App/ArtistMenu';
                break;
            case QA.Enum.ItemMix:
                url = 'Qcm.App/MixMenu';
                break;
            }
            if (!url)
                return;
            MD.Util.showPopup(url, Object.assign({}, props, {
                itemId: id,
                anchorItem: parent
            }), parent);
        }
    }

    Timer {
        running: !win.active
        interval: 10 * 1000
        onTriggered: {
            QA.App.releaseResources(win, {
                pool_obj: MD.Util.poolObjectCount()
            });
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
        sequences: [StandardKey.Quit]
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
        QA.PageStack {
            id: sv_main

            // property var playing_page: {
            //     const page = MD.Util.createItem(comp_playing, {}, sv_main);
            //     page.visible = false;
            //     playing_page = page;
            // }

            clip: true

            initialItem: QA.MainPage {}

            function back() {
                if (currentItem.canBack) {
                    this.currentItem.back();
                } else {
                    this.pop_page(null);
                }
            }

            Connections {
                function onRoute(dst) {
                    if (dst === QA.Enum.SRMain) {
                        sv_main.pop_page(null);
                    }
                }

                target: QA.Action
            }
        }
    }
    Component {
        id: comp_welcome
        QA.WelcomePage {}
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
