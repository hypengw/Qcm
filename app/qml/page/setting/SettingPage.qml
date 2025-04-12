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

        QA.SettingSection {
            spacing: 2
            width: parent.width
            height: implicitHeight
            horizontalPadding: 16

            QA.SettingRow {
                icon.name: MD.Token.icon.palette
                text: 'Theme'
                supportText: ''
                onClicked: {
                    root.MD.MProp.page.pushItem('qrc:/Qcm/App/qml/page/setting/ThemeSetting.qml');
                }
            }
            QA.SettingRow {
                icon.name: MD.Token.icon.media_output
                text: 'Audio'
                supportText: ''
                onClicked: {
                    root.MD.MProp.page.pushItem('qrc:/Qcm/App/qml/page/setting/AudioSetting.qml');
                }
            }
            QA.SettingRow {
                icon.name: MD.Token.icon.router
                text: 'Network'
                supportText: ''
                onClicked:
                // root.MD.MProp.page.pushItem('qrc:/Qcm/App/qml/page/setting/NetworkSetting.qml');
                {}
            }
        }
    }
}
