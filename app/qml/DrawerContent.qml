pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.VerticalFlickable {
    id: root
    property bool standard: false
    property int pageIndex: 0
    property MD.Action rightTopAction: null

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

            Item {
                implicitHeight: 40
            }
        }
        Divider {}

        Repeater {
            model: QA.App.pages
            MD.DrawerItem {
                Layout.fillWidth: true
                required property var modelData
                required property var index
                visible: true //root.MD.MatProp.size.isCompact || root.standard
                checked: root.standard && root.pageIndex == index
                action: MD.Action {
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
            action: MD.Action {
                icon.name: MD.Token.icon.info
                text: qsTr('about')

                onTriggered: {
                    QA.Action.popup_special(QA.Enum.SRAbout);
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
