import QtQuick
import QtCore
import QtQuick.Controls.Basic
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Material as MD
import Qcm.Service.Ncm as NCM

MD.Page {
    id: root
    title: qsTr("playlist")

    // list<string>
    readonly property var cat_list: ['全部歌单', '官方'].concat(custom_cat_list)
    property string cur: '全部歌单'
    property var custom_cat_list: ['华语', '流行', '电子', 'ACG', '欧美', '运动']

    padding: 0
    topPadding: MD.MatProp.size.verticalPadding

    function switchCat(cat) {
        view_container.switchTo(m_list_view, {
            "cat": cat
        }, true);
    }

    Component {
        id: m_list_view
        NCM.PlaylistListView {
            corners: MD.Util.corner(0, root.radius)
        }
    }

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
                corners: MD.Util.corner(root.radius, 0)

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
        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true
            z: -1

            QA.PageContainer {
                id: view_container
                anchors.fill: parent

                initialItem: Item {}
            }
        }
    }

    MD.FAB {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 16
        anchors.bottomMargin: 16
        action: Action {
            icon.name: MD.Token.icon.edit

            onTriggered: {
                QA.Action.popup_page('qrc:/Qcm/Service/Ncm/qml/page/PlaylistCataloguePage.qml', {}, {
                    "fillHeight": true
                }, popup => {
                    popup.closed.connect(() => {
                        settings.read();
                    });
                });
            }
        }
    }
}
