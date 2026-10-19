pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

QA.SettingBasePage {
    id: root
    title: qsTr('theme')

    flickable.contentHeight: m_theme_section.height

    QA.SettingSection {
        id: m_theme_section
        width: parent.width
        height: implicitHeight
        spacing: 2

        QA.SettingRow {
            canInput: false
            text: qsTr('theme mode')
            below: MD.HorizontalListView {
                id: m_theme_mode_view
                expand: true
                spacing: 8
                implicitHeight: 40
                model: ListModel {
                    ListElement {
                        name: qsTr("system")
                    }
                    ListElement {
                        name: qsTr("light")
                    }
                    ListElement {
                        name: qsTr("dark")
                    }
                }
                currentIndex: {
                    const g = QA.Global;
                    if (g.use_system_color_scheme) {
                        return 0;
                    } else {
                        return g.color_scheme == MD.Enum.Light ? 1 : 2;
                    }
                }
                delegate: MD.InputChip {
                    required property int index
                    required property var model
                    icon.name: ''
                    selected: m_theme_mode_view.currentIndex == index
                    text: model.name
                    onSelectionRequested: {
                        const g = QA.Global;
                        if (index == 1) {
                            g.color_scheme = MD.Enum.Light;
                        } else if (index == 2) {
                            g.color_scheme = MD.Enum.Dark;
                        }
                        g.use_system_color_scheme = index == 0;
                    }
                }
            }
        }
        QA.SettingRow {
            canInput: false
            text: qsTr('accent color')
            below: MD.HorizontalListView {
                id: m_accent_color_view
                expand: true
                spacing: 8
                implicitHeight: 40
                model: ListModel {
                    ListElement {
                        name: "system"
                    }
                    ListElement {
                        name: "custom"
                    }
                }
                currentIndex: {
                    const g = QA.Global;
                    if (g.use_system_accent_color) {
                        return 0;
                    } else {
                        return 1;
                    }
                }
                delegate: MD.InputChip {
                    required property int index
                    required property var model
                    icon.name: ''
                    selected: m_accent_color_view.currentIndex == index
                    text: model.name
                    onSelectionRequested: {
                        QA.Global.use_system_accent_color = index == 0;
                        m_theme_section.columnChanged();
                    }
                }
            }
        }
        QA.SettingRow {
            canInput: false
            visible: !QA.Global.use_system_accent_color
            text: qsTr('custom accent color')
            below: ColorSelectRow {}
        }

        QA.SettingRow {
            canInput: false
            text: qsTr('palette')
            below: MD.HorizontalListView {
                id: m_palette_view
                expand: true
                spacing: 8
                implicitHeight: 40
                model: MD.PaletteModel {}
                currentIndex: QA.Global.palette_type
                delegate: MD.InputChip {
                    required property int index
                    required property var model
                    icon.name: ''
                    selected: m_palette_view.currentIndex == index
                    text: model.name
                    onSelectionRequested: QA.Global.palette_type = index
                }
            }
        }
    }

    component ColorSelectRow: ColumnLayout {
        spacing: 8
        Layout.leftMargin: 16
        Layout.rightMargin: 16
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
                    required property var modelData
                    Layout.fillWidth: true
                    implicitHeight: implicitWidth
                    implicitWidth: 24

                    Rectangle {
                        anchors.centerIn: parent
                        color: parent.modelData.value
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
}
