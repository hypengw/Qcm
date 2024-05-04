import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD
import "../js/util.mjs" as Util

MD.Page {
    id: root
    padding: 0

    property int pageIndex: -1
    property var pages: [
        {
            "icon": MD.Token.icon.library_music,
            "name": qsTr('library'),
            "page": 'qrc:/Qcm/App/qml/page/MinePage.qml',
            "cache": true
        },
        {
            "icon": MD.Token.icon.today,
            "name": qsTr('today'),
            "page": 'qrc:/Qcm/App/qml/page/TodayPage.qml',
            "cache": true
        },
        {
            "icon": MD.Token.icon.queue_music,
            "name": qsTr('playlist'),
            "page": 'qrc:/Qcm/App/qml/page/PlaylistListPage.qml'
        },
        {
            "icon": MD.Token.icon.cloud,
            "name": qsTr('cloud'),
            "page": 'qrc:/Qcm/App/qml/page/CloudPage.qml',
            "cache": true
        }
    ]

    canBack: m_page_stack.canBack

    function back() {
        m_page_stack.back();
    }

    Connections {
        function onSig_route(msg) {
            m_page_stack.push_page(msg.qml, msg.props);
        }

        target: QA.Global
    }
    contentItem: Item {
        RowLayout {
            id: m_large_layout
            anchors.fill: parent
            visible: false

            ColumnLayout {
                Layout.topMargin: 12
                Layout.bottomMargin: 12
                Layout.preferredWidth: 56 + 24
                Layout.fillWidth: false
                Layout.fillHeight: true

                StackLayout {
                    Layout.fillHeight: false
                    currentIndex: 1

                    Binding on currentIndex {
                        value: 0
                        when: m_page_stack.depth > 1 || !!page_container.canBack
                    }

                    ColumnLayout {
                        MD.IconButton {
                            Layout.alignment: Qt.AlignHCenter
                            action: Action {
                                icon.name: MD.Token.icon.arrow_back

                                onTriggered: {
                                    if (m_page_stack.depth > 1)
                                        m_page_stack.pop_page();
                                    else if (page_container.canBack)
                                        page_container.back();
                                }
                            }
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                    }
                    MD.ListView {
                        Layout.fillWidth: true
                        implicitHeight: contentHeight
                        interactive: false
                        spacing: 12
                        reuseItems: false
                        currentIndex: root.pageIndex

                        delegate: MD.RailItem {
                            width: ListView.view.width
                            icon.name: model.icon
                            text: model.name
                            checked: root.pageIndex == index
                            onClicked: {
                                if (model.action)
                                    model.action.do();
                                else
                                    root.pageIndex = index;
                            }
                        }

                        model: ListModel {}

                        Component.onCompleted: {
                            const pages = [
                                {
                                    "icon": MD.Token.icon.menu,
                                    "name": qsTr('menu'),
                                    "action": {
                                        "do": function () {
                                            item_drawer.open();
                                        }
                                    }
                                },
                                ...root.pages];
                            if (QA.App.debug) {
                                pages.push({
                                    "icon": MD.Token.icon.queue_music,
                                    "name": qsTr('test'),
                                    "page": 'qrc:/Qcm/App/qml/page/MaterialTest.qml'
                                });
                            }
                            pages.forEach(m => {
                                model.append(m);
                            });
                            root.pageIndex = 1;
                        }
                        onCurrentIndexChanged: {
                            const m = model.get(currentIndex);
                            if (m.page)
                                page_container.switchTo(m.page, m.props, m.cache);
                        }
                    }
                }
                Item {
                    Layout.fillHeight: true
                }
                MD.IconButton {
                    Layout.alignment: Qt.AlignHCenter
                    action: Action {
                        icon.name: MD.Token.icon.search
                        onTriggered: {
                            QA.Global.route('qrc:/Qcm/App/qml/page/SearchPage.qml');
                        }
                    }
                }
                MD.IconButton {
                    Layout.alignment: Qt.AlignHCenter
                    visible: !QA.Global.use_system_color_scheme
                    action: QA.ColorSchemeAction {}
                }
                MD.IconButton {
                    Layout.alignment: Qt.AlignHCenter
                    action: Action {
                        icon.name: MD.Token.icon.settings

                        onTriggered: {
                            QA.Global.show_page_popup('qrc:/Qcm/App/qml/page/SettingsPage.qml', {});
                        }
                    }
                }
            }

            LayoutItemProxy {
                target: m_content
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

            RowLayout {
                visible: m_page_stack.depth <= 1
                Layout.fillWidth: true
                Repeater {
                    model: root.pages
                    Item {
                        Layout.fillWidth: true
                        implicitHeight: 12 + children[0].implicitHeight + 16
                        MD.BarItem {
                            anchors.fill: parent
                            anchors.topMargin: 12
                            anchors.bottomMargin: 16
                            icon.name: modelData.icon
                            text: modelData.name
                            checked: root.pageIndex == index + 1
                            onClicked: {
                                if (modelData.action)
                                    modelData.action.do();
                                else
                                    root.pageIndex = index + 1;
                            }
                        }
                    }
                }
            }
        }
    }

    QA.Drawer {
        id: item_drawer
        width: Math.min(400, QA.Global.main_win.width * 0.8)
        height: root.height
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
                }

                property bool canBack: (currentItem?.canBack ?? false) || depth > 1
                function back() {
                    if (this.currentItem?.canBack) {
                        this.currentItem.back();
                    } else {
                        this.pop_page();
                    }
                }
            }
            QA.PlayBar {
                Layout.fillWidth: true
            }
        }
    }

    onWidthChanged: {
        if (width < 500) {
            m_small_layout.visible = true;
            m_large_layout.visible = false;
        } else {
            m_large_layout.visible = true;
            m_small_layout.visible = false;
        }
    }
}
