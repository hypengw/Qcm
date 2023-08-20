import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qcm.App
import ".."
import "../component"
import "../part"

Page {
    id: root

    // list<string>
    readonly property var cat_list: ['全部歌单', '官方'].concat(custom_cat_list)
    property string cur: '全部歌单'
    property var custom_cat_list: ['华语', '流行', '电子', 'ACG', '欧美', '运动']

    function switchCat(cat) {
        view_container.switchTo('qrc:/Qcm/App/qml/part/PlaylistListView.qml', {
                "cat": cat
            }, true);
    }

    padding: 16

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
                        Material.accent: modelData === cur ? Theme.color.secondary_container : Theme.color.surface_container_low
                        highlighted: true

                        action: Action {
                            text: modelData

                            onTriggered: {
                                cur = modelData;
                            }
                        }
                    }
                }
                MRoundButton {
                    Material.accent: Theme.color.secondary
                    Material.elevation: 1
                    highlighted: true

                    action: Action {
                        icon.name: Theme.ic.edit

                        onTriggered: {
                            const popup = QA.show_page_popup('qrc:/Qcm/App/qml/page/PlaylistCataloguePage.qml', {}, {
                                    "fillHeight": true
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
            Layout.fillHeight: true
            Layout.fillWidth: true
            Material.background: Theme.color.surface
            Material.elevation: 1
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
