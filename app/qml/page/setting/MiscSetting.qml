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
            height: implicitHeight
            spacing: 12
            width: parent.width

            SettingSection {
                id: sec_misc
                Layout.fillWidth: true
                title: qsTr('misc')

                SettingRow {
                    Layout.fillWidth: true
                    font.capitalization: Font.MixedCase
                    text: qsTr('Random Playback Mode')
                    supportText: qsTr('Truly random playback instead of shuffle')

                    actionItem: MD.Switch {
                        checked: QA.App.playqueue.randomMode
                        onCheckedChanged: {
                            QA.App.playqueue.randomMode = checked;
                        }
                    }
                }
            }
        }
    }
}
