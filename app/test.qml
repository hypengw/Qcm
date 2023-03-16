import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Window
import QtQuick.Layouts
import Qt.labs.settings

import QcmApp

ApplicationWindow {
    id: win
    width: 700
    height: 450
    visible: true
    title: QA.user_info.nickname

    Material.primary: Theme.color.primary
    Material.accent: Material.primary
    Material.background: Theme.color.background
    Material.foreground: Theme.color.on_background
    Material.theme: Theme.is_dark_theme ? Material.Dark : Material.Light

    color: Material.background

    Settings {
        category: 'window'
        property alias x: win.x
        property alias y: win.y
        property alias width: win.width
        property alias height: win.height
    }

    StackView {
        id: win_stack
        anchors.fill: parent
        initialItem: Item {}
        Connections {
            target: QA.querier_user
            function onStatusChanged() {
                if (target.status !== ApiQuerierBase.Finished)
                    return
                win_stack.replace(
                            win_stack.currentItem,
                            QA.is_login ? main_view : 'qrc:/QcmApp/qml/LoginPage.qml')
            }
        }
    }

    Component {
        id: main_view
        RowLayout {
            spacing: 0
            Pane {
                z: 1
                Layout.fillHeight: true
                Material.elevation: 2
                Material.background: Theme.color.surface_2
                padding: 0
                ColumnLayout {
                    anchors.fill: parent
                    ListView {
                        Layout.fillWidth: true
                        implicitHeight: contentHeight
                        currentIndex: 1
                        model: ListModel {}
                        delegate: MItemDelegate {
                            width: ListView.view.width
                            Material.primary: Theme.color.tertiary
                            Label {
                                anchors.centerIn: parent
                                font.family: Theme.font.icon_round.family
                                font.pointSize: 16
                                text: model.icon
                            }
                            onClicked: {
                                if (model.action) {
                                    model.action()
                                    return
                                }
                                ListView.view.currentIndex = index
                                page_container.switchTo(model.page, {},
                                                        model.cache)
                            }
                        }
                        Component.onCompleted: {
                            const base = ({});

                                       ([{
                                             "icon": Theme.ic.menu,
                                             "action": () => {}
                                         }, {
                                             "icon": Theme.ic.library_music,
                                             "page": 'qrc:/QcmApp/qml/page/MinePage.qml',
                                             "cache": true
                                         }, {
                                             "icon": Theme.ic.today,
                                             "page": 'qrc:/QcmApp/qml/page/TodayPage.qml',
                                             "cache": false
                                         }].map(m => model.append(m)))
                        }
                    }
                    Item {
                        Layout.fillHeight: true
                    }
                    RoundButton {
                        font.family: Theme.font.icon_round.family
                        font.pointSize: 16
                        flat: true
                        text: Theme.is_dark_theme ? Theme.ic.dark_mode : Theme.ic.light_mode
                        onClicked: {
                            Theme.theme = Theme.is_dark_theme ? MdColorMgr.Light : MdColorMgr.Dark
                        }
                    }
                    RoundButton {
                        font.family: Theme.font.icon_round.family
                        font.pointSize: 16
                        flat: true
                        text: Theme.ic.settings
                    }
                }
            }
            ColumnLayout {
                spacing: 0
                // clip: true
                StackView {
                    id: page_stack
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    initialItem: PageContainer {
                        id: page_container
                        initialItem: Item {}
                    }
                }
                PlayBar {
                    Layout.fillWidth: true
                }
            }
        }
    }
}
