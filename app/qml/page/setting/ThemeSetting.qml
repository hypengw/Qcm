pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    font.capitalization: Font.Capitalize
    title: qsTr('theme')
    bottomPadding: radius
    scrolling: !m_flick.atYBeginning

    MD.VerticalFlickable {
        id: m_flick
        anchors.fill: parent
        leftMargin: 0
        rightMargin: 0
        contentHeight: m_theme_section.implicitHeight

        QA.SettingSection {
            id: m_theme_section
            width: parent.width
            height: implicitHeight
            spacing: 2
            horizontalPadding: 16

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
                    MD.ActionGroup {
                        id: m_theme_mode_group
                    }
                    delegate: MD.InputChip {
                        required property int index
                        required property var model
                        action: MD.Action {
                            T.ActionGroup.group: m_theme_mode_group
                            icon.name: ''
                            checkable: false
                            checked: m_theme_mode_view.currentIndex == index
                            text: model.name
                            onTriggered: {
                                m_theme_mode_view.currentIndex = index;
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
                    MD.ActionGroup {
                        id: m_color_group
                        onTriggered: act => {
                            const g = QA.Global;
                            g.use_system_accent_color = actions[0] == act;
                            m_theme_section.columnChanged();
                        }
                    }
                    delegate: MD.InputChip {
                        required property int index
                        required property var model
                        action: MD.Action {
                            T.ActionGroup.group: m_color_group
                            icon.name: ''
                            checkable: true
                            checked: m_accent_color_view.currentIndex == index
                            text: model.name
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
                    MD.ActionGroup {
                        id: m_palette_group
                    }
                    delegate: MD.InputChip {
                        required property int index
                        required property var model
                        action: MD.Action {
                            T.ActionGroup.group: m_palette_group
                            icon.name: ''
                            checkable: true
                            checked: m_palette_view.currentIndex == index
                            onCheckedChanged: function (c) {
                                if (c) {
                                    const g = QA.Global;
                                    g.palette_type = index;
                                }
                            }
                            text: model.name
                        }
                    }
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
