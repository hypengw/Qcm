pragma ComponentBehavior: Bound
import QtCore
import QtQuick
import QtQuick.Window
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0

    property int pageIndex: -1
    showHeader: false

    title: m_page_stack.currentItem?.title ?? ""
    canBack: m_page_stack.canBack

    backgroundColor: MD.MProp.backgroundColor

    property var model: QA.App.pages
    readonly property bool useLarge: MD.MProp.size.windowClass >= MD.Enum.WindowClassLarge

    function back() {
        m_page_stack.back();
    }

    onPageIndexChanged: {
        const m = model.item(pageIndex);
        if (m?.source) {
            page_container.switchTo(m.source, m.props ?? {}, m.cache);
        }
    }

    Component.onCompleted: {
        root.pageIndex = 0;
    }

    Connections {
        function onRoute(msg) {
            m_page_stack.push_page(msg.dst, msg.props);
        }

        function onRouteMain(idx) {
            root.pageIndex = idx;
        }
        target: QA.Action
    }

    contentItem: Item {
        ColumnLayout {
            id: m_large_layout
            anchors.fill: parent
            visible: false

            RowLayout {
                MD.StandardDrawer {
                    id: m_drawer
                    Layout.fillHeight: true
                    model: root.model
                    onClicked: function (model) {
                        m_page_stack.pop_page(null);
                        QA.Action.routeMain(model.index);
                    }

                    Behavior on implicitWidth {
                        NumberAnimation {
                            duration: 200
                        }
                    }

                    headerAction: (m_page_stack.depth > 1 || !!page_container.canBack) ? m_drawer_back_action : defaultHeaderAction
                    MD.Action {
                        id: m_drawer_back_action
                        icon.name: MD.Token.icon.arrow_back
                        onTriggered: {
                            if (m_page_stack.depth > 1)
                                m_page_stack.pop_page();
                            else if (page_container.canBack)
                                page_container.back();
                        }
                    }

                    footer: Column {
                        MD.IconButton {
                            anchors.horizontalCenter: parent.horizontalCenter
                            action: MD.Action {
                                icon.name: MD.Token.icon.hard_drive
                                onTriggered: {
                                    QA.Action.openPopup(QA.Enum.SRSync);
                                }
                            }
                        }
                        MD.IconButton {
                            anchors.horizontalCenter: parent.horizontalCenter
                            action: QA.SettingAction {}
                        }
                    }
                    drawerContent: Item {
                        implicitHeight: children[0].implicitHeight
                        Column {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: parent.width - 24
                            MD.DrawerItem {
                                width: parent.width
                                action: MD.Action {
                                    icon.name: MD.Token.icon.hard_drive
                                    text: qsTr('provider')
                                    onTriggered: {
                                        QA.Action.openPopup(QA.Enum.SRSync);
                                        m_drawer.close();
                                    }
                                }
                            }
                            MD.DrawerItem {
                                width: parent.width
                                action: QA.SettingAction {
                                    onTriggered: {
                                        m_drawer.close();
                                    }
                                }
                            }
                            MD.DrawerItem {
                                width: parent.width
                                action: MD.Action {
                                    icon.name: MD.Token.icon.info
                                    text: qsTr('about')

                                    onTriggered: {
                                        QA.Action.openPopup(QA.Enum.SRAbout);
                                        m_drawer.close();
                                    }
                                }
                            }
                        }
                    }
                }

                LayoutItemProxy {
                    target: m_content
                }
            }
            LayoutItemProxy {
                Layout.fillWidth: true
                z: m_play_bar_flick.closed ? -99 : 99
                target: m_play_bar
            }
        }
        ColumnLayout {
            id: m_small_layout
            visible: false
            anchors.fill: parent
            spacing: 0

            LayoutItemProxy {
                target: m_content
            }

            LayoutItemProxy {
                Layout.fillWidth: true
                z: m_play_bar_flick.closed ? -99 : 1
                target: m_play_bar
            }

            MD.Pane {
                Layout.fillWidth: true
                visible: m_page_stack.depth <= 1
                padding: 0
                backgroundColor: MD.MProp.color.surface_container
                elevation: MD.Token.elevation.level2
                RowLayout {
                    anchors.fill: parent
                    Repeater {
                        model: root.model
                        Item {
                            Layout.fillWidth: true
                            implicitHeight: 12 + children[0].implicitHeight + 16
                            required property var model
                            required property int index
                            MD.BarItem {
                                anchors.fill: parent
                                anchors.topMargin: 12
                                anchors.bottomMargin: 16
                                icon.name: parent.model.icon
                                text: parent.model.name
                                checked: root.pageIndex == parent.index
                                onClicked: {
                                    QA.Action.routeMain(parent.index);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    Item {
        visible: false

        Item {
            id: m_play_bar
            implicitHeight: children[0].implicitHeight
            property point origin: Qt.point(0, 0)
            function updateOrigin() {
                const p = m_play_bar.mapFromGlobal(0, 0);
                origin = p;
            }
            onParentChanged: updateOrigin()
            onYChanged: updateOrigin()

            MD.InputBlock {
                target: m_play_bar_flick
                acceptWheel: !m_play_bar_flick.closed
                acceptHover: true
                acceptTouch: true
                acceptMouseButtons: Qt.LeftButton
            }

            QA.PlayBarFlickable {
                id: m_play_bar_flick
                y: m_play_bar.origin.y
                width: parent.width
                height: windowHeight
                onHeightChanged: m_play_bar.updateOrigin()
            }
        }

        ColumnLayout {
            id: m_content
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 0
            QA.PageStack {
                id: m_page_stack
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.topMargin: 0
                Layout.bottomMargin: 0
                Layout.rightMargin: 16
                clip: true

                initialItem: QA.PageContainer {
                    id: page_container
                    initialItem: Item {}
                    property string title: currentItem?.title ?? ""

                    MD.MProp.page: m_page_context
                    MD.PageContext {
                        id: m_page_context
                        headerType: MD.Enum.AppBarCenterAligned
                        showHeader: root.MD.MProp.size.isCompact
                        leadingAction: root.canBack ? m_back_action : m_draw_action
                        radius: root.radius
                    }
                }

                MD.MProp.backgroundColor: MD.MProp.color.surface
                MD.MProp.page: m_page_stack_context
                MD.PageContext {
                    id: m_page_stack_context
                    leadingAction: root.canBack ? m_back_action : null
                    showHeader: root.MD.MProp.size.isCompact
                    radius: root.radius
                }

                MD.Action {
                    id: m_back_action
                    icon.name: MD.Token.icon.arrow_back
                    onTriggered: {
                        if (root.canBack)
                            root.back();
                    }
                }
                MD.Action {
                    id: m_draw_action
                    icon.name: MD.Token.icon.menu
                    onTriggered: {
                        m_drawer.open();
                    }
                }

                Binding {
                    when: root.MD.MProp.size.isCompact
                    m_page_stack.Layout.topMargin: 0
                    m_page_stack.Layout.bottomMargin: 0
                    m_page_stack.Layout.rightMargin: 0
                }
            }
        }
    }

    Connections {
        target: root.MD.MProp.size
        function onWindowClassChanged() {
            const size = root.MD.MProp.size;
            if (size.isCompact) {
                m_small_layout.visible = true;
                m_large_layout.visible = false;
            } else {
                m_large_layout.visible = true;
                m_small_layout.visible = false;
            }
        }
        Component.onCompleted: {
            this.onWindowClassChanged();
        }
    }
}
