import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Drawer {
    id: root
    font.capitalization: Font.Capitalize
    contentItem: MD.Flickable {
        ColumnLayout {
            height: implicitHeight
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 0

            ColumnLayout {
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                spacing: 16

                QA.Image {
                    property int size: 48
                    Layout.preferredWidth: size
                    Layout.preferredHeight: size
                    radius: size / 2
                    source: `image://ncm/${QA.Global.user_info.avatarUrl}`
                }

                RowLayout {
                    MD.Text {
                        Layout.alignment: Qt.AlignVCenter
                        typescale: {
                            const s = MD.Token.typescale.label_large.fresh();
                            s.weight = Font.Bold;
                            return s;
                        }
                        text: QA.Global.user_info.nickname
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    MD.IconButton {
                        id: m_expand
                        checkable: true
                        action: Action {
                            icon.name: MD.Token.icon.keyboard_arrow_down
                            onTriggered: {}
                        }
                    }

                    MD.IconButton {
                        action: Action {
                            icon.name: MD.Token.icon.logout
                            onTriggered: {
                                root.close();
                                qr_logout.logout();
                            }
                        }
                    }
                }
            }

            MD.Divider {
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 8
            }

            MD.ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: contentHeight
                model: QA.Global.userModel
                visible: m_expand.checked
                delegate: MD.ListItem {
                    required property var model
                    required property int index
                    width: ListView.view.contentWidth
                    text: model.nickname
                }
                //footer: MD.ListItem {
                //    width: ListView.view.contentWidth
                //    height: implicitHeight
                //    text: qsTr('add account')
                //    font.capitalization: Font.Capitalize

                //    leader: MD.Icon {
                //        name: MD.Token.icon.add_circle
                //    }
                //}
            }
            MD.Divider {
                visible: m_expand.checked
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 8
            }

            MD.DrawerItem {
                Layout.fillWidth: true
                font.capitalization: Font.Capitalize
                action: Action {
                    icon.name: MD.Token.icon.settings
                    text: qsTr('settings')

                    onTriggered: {
                        QA.Global.show_page_popup('qrc:/Qcm/App/qml/page/SettingsPage.qml', {});
                        root.close();
                    }
                }
            }

            MD.DrawerItem {
                Layout.fillWidth: true
                font.capitalization: Font.Capitalize
                action: Action {
                    icon.name: MD.Token.icon.info
                    text: qsTr('about')

                    onTriggered: {
                        QA.Global.show_page_popup('qrc:/Qcm/App/qml/page/AboutPage.qml', {});
                        root.close();
                    }
                }
            }
        }
    }
    QNcm.LogoutQuerier {
        id: qr_logout
        function logout() {
            query();
        }
        onStatusChanged: {
            if (status === QA.enums.Finished) {
                QA.Global.querier_user.query();
            }
        }

        autoReload: false
    }
}
