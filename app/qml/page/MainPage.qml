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
    canBack: !m_play_bar_flick.closed || m_page_stack.canBack

    backgroundColor: MD.MProp.backgroundColor

    property var model: QA.App.pages
    readonly property int pageLeftMargin: 4
    readonly property int pageRightMargin: 12
    readonly property int pageTopMargin: MD.MProp.size.isCompact ? 0 : MD.MProp.size.verticalPadding
    readonly property real playBarMargin: 12
    readonly property real bottomNavigationHeight: MD.MProp.size.isCompact && bottomNavigation.visible ? bottomNavigation.height : 0
    readonly property real playBarInset: Math.max(0, m_play_bar_flick.height - bottomNavigationHeight - m_play_bar_flick.closedBarRect.y) * m_play_bar_flick.presentationProgress
    readonly property int pageBottomMargin: (MD.MProp.size.isCompact ? 0 : MD.MProp.size.verticalPadding) + Math.ceil(playBarInset)

    function back() {
        if (!m_play_bar_flick.closed)
            m_play_bar_flick.back();
        else
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
        Item {
            id: backgroundRoot
            objectName: "playbackBackground"
            anchors.fill: parent
            Rectangle {
                anchors.fill: parent
                color: root.backgroundColor
            }
            ColumnLayout {
                id: m_large_layout
                anchors.fill: parent
                visible: !root.MD.MProp.size.isCompact

                RowLayout {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    MD.NavigationRail {
                        id: m_drawer
                        Layout.fillHeight: true
                        bottomPadding: root.playBarInset
                        model: root.model
                        hideWhenCollapsed: true

                        Binding {
                            target: m_drawer
                            property: "currentIndex"
                            value: root.pageIndex
                        }

                        onCurrentIndexChangeRequested: function (index) {
                            m_page_stack.pop_page(null);
                            QA.Action.routeMain(index);
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
                                selected: false
                                icon.name: MD.Token.icon.hard_drive
                                iconStyle: m_drawer.useLarge ? MD.Enum.IconAndText : MD.Enum.IconOnly
                                text: qsTr('provider')
                                onSelectionRequested: {
                                    QA.Action.openPopup(QA.Enum.SRSync);
                                    if (m_drawer.useModal)
                                        m_drawer.close();
                                }
                            }
                            MD.RailItem {
                                width: parent.width
                                expand: m_drawer.useLarge
                                selected: false
                                iconStyle: m_drawer.useLarge ? MD.Enum.IconAndText : MD.Enum.IconOnly
                                readonly property QA.SettingAction settingsAction: QA.SettingAction {}
                                text: settingsAction.text
                                icon.actionIcon: settingsAction.icon
                                enabled: settingsAction.enabled
                                onSelectionRequested: {
                                    settingsAction.trigger();
                                    if (m_drawer.useModal)
                                        m_drawer.close();
                                }
                            }
                            MD.RailItem {
                                visible: m_drawer.useLarge
                                width: parent.width
                                expand: true
                                selected: false
                                icon.name: MD.Token.icon.info
                                text: qsTr('about')
                                onSelectionRequested: {
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
            }
            ColumnLayout {
                id: m_small_layout
                visible: root.MD.MProp.size.isCompact
                anchors.fill: parent
                spacing: 0

                LayoutItemProxy {
                    target: m_content
                }

                MD.Pane {
                    id: bottomNavigation
                    objectName: "bottomNavigation"
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
                                    selected: root.pageIndex == parent.index
                                    onSelectionRequested: {
                                        QA.Action.routeMain(parent.index);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        QA.PlayBarFlickable {
            id: m_play_bar_flick
            anchors.fill: parent
            backdropSource: backgroundRoot
            barMargin: root.playBarMargin
            closedBarRect: Qt.rect(root.playBarMargin, height - root.bottomNavigationHeight - barHeight - root.playBarMargin, Math.max(0, width - 2 * root.playBarMargin), barHeight)
        }
    }
    Item {
        visible: false

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
                        bottomInset: Math.ceil(root.playBarInset)
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
                    bottomInset: Math.ceil(root.playBarInset)
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
}
