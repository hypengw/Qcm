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
                id: sec_app
                Layout.fillWidth: true
                title: qsTr('theme')

                ColumnLayout {
                    spacing: 8
                    visible: !QA.Global.use_system_accent_color
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    MD.Text {
                        text: qsTr('theme color')
                        typescale: MD.Token.typescale.title_small
                        font.capitalization: Font.Capitalize
                    }
                    RowLayout {
                        Repeater {
                            model: [
                                {
                                    "name": 'red',
                                    "value": '#F44336'
                                },
                                {
                                    "name": 'pink',
                                    "value": '#E91E63'
                                },
                                {
                                    "name": 'purple',
                                    "value": '#9C27B0'
                                },
                                {
                                    "name": 'indigo',
                                    "value": '#3F51B5'
                                },
                                {
                                    "name": 'blue',
                                    "value": '#2196F3'
                                },
                                {
                                    "name": 'cyan',
                                    "value": '#00BCD4'
                                },
                                {
                                    "name": 'orange',
                                    "value": '#FF9800'
                                },
                            ]

                            delegate: Item {
                                id: color_item
                                Layout.fillWidth: true
                                implicitHeight: implicitWidth
                                implicitWidth: 24

                                Rectangle {
                                    anchors.centerIn: parent
                                    color: modelData.value
                                    height: width
                                    radius: width / 2
                                    width: parent.implicitWidth

                                    Rectangle {
                                        anchors.centerIn: parent
                                        border.color: MD.MProp.backgroundColor
                                        border.width: 3
                                        color: parent.color
                                        height: width
                                        radius: width / 2
                                        visible: color === QA.Global.primary_color
                                        width: 18
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        hoverEnabled: true

                                        onClicked: {
                                            QA.Global.primary_color = modelData.value;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                SettingRow {
                    Layout.fillWidth: true
                    text: qsTr('follow system color')
                    trailing: MD.Switch {
                        checked: QA.Global.use_system_accent_color
                        onCheckedChanged: QA.Global.use_system_accent_color = checked
                    }
                }
                SettingRow {
                    Layout.fillWidth: true
                    text: qsTr('follow system night mode')
                    trailing: MD.Switch {
                        checked: QA.Global.use_system_color_scheme
                        onCheckedChanged: QA.Global.use_system_color_scheme = checked
                    }
                }
            }
        }
    }
}
