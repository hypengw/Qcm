import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qt.labs.settings
import QcmApp
import ".."
import "../component"
import "../part"

Page {
    id: root

    property string cur: '全部歌单'
    // list<string>
    readonly property var cat_list: ['全部歌单', '官方'].concat(custom_cat_list)
    property var custom_cat_list: ['华语', '流行', '电子', 'ACG', '欧美', '运动']

    function switchCat(cat) {
        view_container.switchTo('qrc:/QcmApp/qml/part/PlaylistListView.qml', {
            "cat": cat
        }, true);
    }

    onCurChanged: {
        switchCat(cur);
    }
    padding: 16
    Component.onCompleted: {
        curChanged();
    }

    Settings {
        id: settings

        property alias cat_list: root.custom_cat_list

        function read() {
            root.custom_cat_list = value('cat_list', root.custom_cat_list);
        }

        category: QA.user_setting_category
    }

    ColumnLayout {
        anchors.fill: parent

        Pane {
            Layout.fillWidth: true

            Flow {
                anchors.fill: parent
                spacing: 8

                Repeater {
                    model: root.cat_list

                    delegate: MButton {
                        highlighted: true
                        Material.accent: modelData === cur ? Theme.color.primary : Theme.color.surface_2

                        action: Action {
                            text: modelData
                            onTriggered: {
                                cur = modelData;
                            }
                        }

                    }

                }

                MRoundButton {
                    highlighted: true
                    Material.elevation: 1
                    Material.accent: Theme.color.secondary

                    action: Action {
                        icon.name: Theme.ic.edit
                        onTriggered: {
                            const popup = QA.show_page_popup('qrc:/QcmApp/qml/page/PlaylistCataloguePage.qml', {
                            });
                            popup.closed.connect(() => {
                                settings.read();
                            });
                        }
                    }

                }

            }

        }

        Pane {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Material.elevation: 1
            Material.background: Theme.color.surface_1
            padding: 0

            PageContainer {
                id: view_container

                anchors.fill: parent

                initialItem: Item {
                }

            }

        }

    }

}
