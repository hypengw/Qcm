import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Page {
    id: root

    // list<string>
    readonly property var cat_list: ['全部歌单', '官方'].concat(custom_cat_list)
    property string cur: '全部歌单'
    property var custom_cat_list: ['华语', '流行', '电子', 'ACG', '欧美', '运动']

    function switchCat(cat) {
        view_container.switchTo('qrc:/Qcm/App/qml/component/PlaylistListView.qml', {
                "cat": cat
            }, true);
    }

    padding: 0

    Component.onCompleted: {
        curChanged();
    }
    onCurChanged: {
        switchCat(cur);
    }

    Settings {
        id: settings

        property alias cat_list: root.custom_cat_list

        function read() {
            root.custom_cat_list = value('cat_list', root.custom_cat_list);
        }

        category: QA.Global.user_setting_category
    }
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {

            MD.TabBar {
                id: item_bar
                Layout.fillWidth: true
                spacing: 0
                clip: true

                readonly property int tab_width: parent.width / root.cat_list.length

                Component.onCompleted: {
                    currentIndexChanged();
                }
                Repeater {
                    model: root.cat_list.length

                    MD.TabButton {
                        required property int index
                        width: Math.max(item_bar.tab_width, implicitWidth)
                        action: Action {
                            text: root.cat_list[index]

                            onTriggered: {
                                item_bar.currentIndex = index;
                                root.cur = root.cat_list[index];
                            }
                        }
                    }
                }
            }

        }
        MD.Pane {
            Layout.fillHeight: true
            Layout.fillWidth: true
            MD.MatProp.backgroundColor: MD.Token.color.surface
            padding: 0

            QA.PageContainer {
                id: view_container
                anchors.fill: parent

                initialItem: Item {
                }
            }
        }
    }

    MD.FAB {
        action: Action {
            icon.name: MD.Token.icon.edit

            onTriggered: {
                const popup = QA.Global.show_page_popup('qrc:/Qcm/App/qml/page/PlaylistCataloguePage.qml', {}, {
                        "fillHeight": true
                    });
                popup.closed.connect(() => {
                        settings.read();
                    });
            }
        }
    }
}
