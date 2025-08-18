import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    font.capitalization: Font.Capitalize
    title: qsTr('settings')
    bottomPadding: radius
    scrolling: !m_flick.atYBeginning

    MD.VerticalFlickable {
        id: m_flick
        anchors.fill: parent
        leftMargin: 0
        rightMargin: 0

        ColumnLayout {
            width: parent.width
            height: implicitHeight
            QA.SettingSection {
                Layout.fillWidth: true
                spacing: 2
                horizontalPadding: 16
                QA.SettingRow {
                    icon.name: MD.Token.icon.hard_drive
                    text: 'provider manage'
                    supportText: ''
                    onClicked: {
                        root.MD.MProp.page.pushItem('Qcm.App/ProviderManagePage');
                    }
                }
            }

            MD.Divider {}

            QA.SettingSection {
                Layout.fillWidth: true
                spacing: 2
                horizontalPadding: 16
                QA.SettingRow {
                    icon.name: MD.Token.icon.palette
                    text: qsTr('Theme')
                    supportText: ''
                    onClicked: {
                        root.MD.MProp.page.pushItem('Qcm.App/ThemeSetting');
                    }
                }
                QA.SettingRow {
                    icon.name: MD.Token.icon.media_output
                    text: qsTr('Audio')
                    supportText: ''
                    onClicked: {
                        root.MD.MProp.page.pushItem('Qcm.App/AudioSetting');
                    }
                }
                QA.SettingRow {
                    icon.name: MD.Token.icon.router
                    text: qsTr('Network')
                    supportText: ''
                    onClicked: {
                        QA.Action.toast(qsTr('Work in progress'));
                    }
                }
                QA.SettingRow {
                    icon.name: MD.Token.icon.storage
                    text: qsTr('Storage')
                    supportText: ''
                    onClicked: {
                        root.MD.MProp.page.pushItem('Qcm.App/StorageSetting');
                    }
                }
            }
        }
    }
}
