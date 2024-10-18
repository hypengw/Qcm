import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Flickable {
    id: root
    property bool standard: false
    property int pageIndex: 0
    property Action rightTopAction: null

    signal close

    topMargin: 16
    bottomMargin: 16

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

            RowLayout {
                QA.Image {
                    property int size: 48
                    Layout.preferredWidth: size
                    Layout.preferredHeight: size
                    radius: size / 2
                    source: QA.Util.image_url(QA.Global.session.user.avatarUrl)
                }
                Item {
                    Layout.fillWidth: true
                }
                MD.IconButton {
                    visible: action
                    action: root.rightTopAction
                }
            }

            RowLayout {
                MD.Text {
                    Layout.alignment: Qt.AlignVCenter
                    typescale: {
                        const s = MD.Token.typescale.label_large.fresh();
                        s.weight = Font.Bold;
                        return s;
                    }
                    text: QA.Global.session.user.nickname
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
                            QA.Action.logout();
                        }
                    }
                }
            }
        }
        Divider {}

        MD.ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: contentHeight
            interactive: false
            model: QA.Global.userModel
            visible: m_expand.checked
            delegate: MD.ListItem {
                required property var model
                required property int index
                width: ListView.view.contentWidth
                text: model.nickname
                mdState.backgroundColor: root.MD.MatProp.backgroundColor
                onClicked: {
                    QA.Action.switch_user(model.userId);
                    root.close();
                }
            }
            footer: MD.ListItem {
                width: ListView.view.contentWidth
                height: implicitHeight
                text: qsTr('add account')
                font.capitalization: Font.Capitalize
                mdState.backgroundColor: root.MD.MatProp.backgroundColor

                leader: MD.Icon {
                    name: MD.Token.icon.add_circle
                }

                onClicked: {
                    QA.Action.route_special(QA.enums.SRLogin);
                    root.close();
                }
            }
        }
        Divider {
            visible: m_expand.checked
        }

        Repeater {
            model: QA.Global.session.pages.filter(el => root.standard || !el.primary)
            MD.DrawerItem {
                Layout.fillWidth: true
                visible: root.MD.MatProp.size.isCompact || root.standard
                checked: root.standard && root.pageIndex == index
                action: Action {
                    icon.name: MD.Token.icon[modelData.icon]
                    text: modelData.name
                    onTriggered: {
                        if (root.standard) {
                            QA.Action.switch_main_page(index);
                        } else {
                            QA.Action.popup_page(modelData.source, {});
                            root.close();
                        }
                    }
                }
            }
        }
        Divider {
            visible: root.MD.MatProp.size.isCompact || root.standard
        }

        MD.DrawerItem {
            Layout.fillWidth: true

            action: QA.SettingAction {
                onTriggered: {
                    root.close();
                }
            }
        }

        MD.DrawerItem {
            Layout.fillWidth: true
            action: Action {
                icon.name: MD.Token.icon.info
                text: qsTr('about')

                onTriggered: {
                    QA.Action.popup_special(QA.enums.SRAbout);
                    root.close();
                }
            }
        }
    }
    component Divider: MD.Divider {
        Layout.topMargin: 8
        Layout.leftMargin: 16
        Layout.rightMargin: 16
        Layout.bottomMargin: 8
    }
}
