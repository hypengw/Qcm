pragma ComponentBehavior: Bound
import QtCore
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

QA.SettingBasePage {
    id: root
    title: qsTr('queue')
    QA.SettingSection {
        width: parent.width
        height: implicitHeight
        spacing: 2

        QA.SettingRow {
            text: qsTr('Random Playback Mode')
            supportText: qsTr('Truly random playback instead of shuffle')
            trailing: MD.Switch {
                checked: QA.App.playqueue.randomMode
                onCheckedChanged: {
                    QA.App.playqueue.randomMode = checked;
                }
            }
        }
    }
}
