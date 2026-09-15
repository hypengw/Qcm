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
    readonly property int pageLeftMargin: 4
    readonly property int pageRightMargin: 12
    readonly property int pageTopMargin: MD.MProp.size.isCompact ? 0 : MD.MProp.size.verticalPadding
    readonly property int pageBottomMargin: MD.MProp.size.isCompact ? 0 : MD.MProp.size.verticalPadding

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
            if (msg instanceof QA.rmsg) {
                m_page_stack.push_page(msg.dst, msg.props);
            }
        }

        function onOpenDrawer() {
            m_drawer.open();
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
                MD.NavigationRail {
                    id: m_drawer
                    Layout.fillHeight: true
                    model: root.model
                    hideWhenCollapsed: true

                    Binding {
                        target: m_drawer
                        property: "currentIndex"
                        value: root.pageIndex
                    }

                    onClicked: function (model) {
                        m_page_stack.pop_page(null);
                        QA.Action.routeMain(model.index);
                    }

                    header: Item {
                        implicitWidth: m_drawer.useLarge ? m_drawer.expandedWidth : m_drawer.collapsedWidth
                        implicitHeight: m_logo.y + m_logo.height + 12

                        MD.StandardIconButton {
                            id: m_menu_button
                            x: m_drawer.useLarge ? (32 - (width - 24) / 2) : (m_drawer.collapsedWidth - width) / 2
                            y: 4
                            icon.name: root.canBack ? MD.Token.icon.arrow_back : (m_drawer.useLarge ? MD.Token.icon.menu_open : MD.Token.icon.menu)
                            onClicked: {
                                if (root.canBack)
                                    root.back();
                                else
                                    m_drawer.toggle();
                            }

                            Behavior on x {
                                NumberAnimation {
                                    duration: MD.Token.duration.long2
                                    easing: MD.Token.easing.emphasized
                                }
                            }
                        }
                        Image {
                            id: m_logo
                            width: 32
                            height: 32
                            x: m_drawer.useLarge ? 32 : (m_drawer.collapsedWidth - width) / 2
                            y: m_menu_button.y + m_menu_button.height + 16
                            source: "qrc:/Qcm/App/assets/Qcm.svg"
                            fillMode: Image.PreserveAspectFit
                            sourceSize: Qt.size(64, 64)

                            Behavior on x {
                                NumberAnimation {
                                    duration: MD.Token.duration.long2
                                    easing: MD.Token.easing.emphasized
                                }
                            }
                        }
                        MD.Label {
                            visible: m_drawer.useLarge
                            anchors.left: m_logo.right
                            anchors.leftMargin: 12
                            anchors.verticalCenter: m_logo.verticalCenter
                            text: "Qcm"
                            typescale: MD.Token.typescale.title_large
                        }
                    }

                    footer: Column {
                        spacing: m_drawer.useLarge ? 0 : 12

                        MD.RailItem {
                            width: parent.width
                            expand: m_drawer.useLarge
                            checked: false
                            icon.name: MD.Token.icon.hard_drive
                            iconStyle: m_drawer.useLarge ? MD.Enum.IconAndText : MD.Enum.IconOnly
                            text: qsTr('provider')
                            onClicked: {
                                QA.Action.openPopup(QA.Enum.SRSync);
                                if (m_drawer.useModal)
                                    m_drawer.close();
                            }
                        }
                        MD.RailItem {
                            width: parent.width
                            expand: m_drawer.useLarge
                            checked: false
                            iconStyle: m_drawer.useLarge ? MD.Enum.IconAndText : MD.Enum.IconOnly
                            action: QA.SettingAction {}
                            onClicked: {
                                if (m_drawer.useModal)
                                    m_drawer.close();
                            }
                        }
                        MD.RailItem {
                            visible: m_drawer.useLarge
                            width: parent.width
                            expand: true
                            checked: false
                            icon.name: MD.Token.icon.info
                            text: qsTr('about')
                            onClicked: {
                                QA.Action.openPopup(QA.Enum.SRAbout);
                                if (m_drawer.useModal)
                                    m_drawer.close();
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
                acceptWheel: false
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
                        leftMargin: root.pageLeftMargin
                        rightMargin: root.pageRightMargin
                        topMargin: root.pageTopMargin
                        bottomMargin: root.pageBottomMargin
                    }
                }

                MD.MProp.backgroundColor: MD.MProp.color.surface
                MD.MProp.page: m_page_stack_context
                MD.PageContext {
                    id: m_page_stack_context
                    leadingAction: root.canBack ? m_back_action : null
                    showHeader: root.MD.MProp.size.isCompact
                    radius: root.radius
                    leftMargin: root.pageLeftMargin
                    rightMargin: root.pageRightMargin
                    topMargin: root.pageTopMargin
                    bottomMargin: root.pageBottomMargin
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
                        QA.Action.openDrawer();
                    }
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
