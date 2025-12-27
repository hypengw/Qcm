import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

QA.SettingBasePage {
    id: root
    title: qsTr('settings')

    ColumnLayout {
        width: parent.width
        height: implicitHeight
        QA.SettingSection {
            Layout.fillWidth: true
            spacing: 2
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
                icon.name: MD.Token.icon.queue_music
                text: qsTr('Queue')
                supportText: ''
                onClicked: {
                    root.MD.MProp.page.pushItem('Qcm.App/QueueSetting');
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
