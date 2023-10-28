import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD
import "../js/util.mjs" as Util

MD.Page {
    id: root
    font.capitalization: Font.Capitalize
    title: qsTr('settings')

    MD.MatProp.textColor: MD.Token.color.on_surface
    MD.MatProp.elevation: MD.Token.elevation.level0

    MD.Flickable {
        id: flick
        anchors.fill: parent
        leftMargin: 0
        rightMargin: 0

        ColumnLayout {
            height: implicitHeight
            spacing: 8
            width: parent.width

            SettingSection {
                id: sec_app
                Layout.fillWidth: true
                title: qsTr('application')

                ColumnLayout {
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    ColumnLayout {
                        spacing: 8

                        MD.Text {
                            text: qsTr('Theme')
                        }
                        RowLayout {
                            Repeater {
                                model: [{
                                        "name": 'red',
                                        "value": '#F44336'
                                    }, {
                                        "name": 'pink',
                                        "value": '#E91E63'
                                    }, {
                                        "name": 'purple',
                                        "value": '#9C27B0'
                                    }, {
                                        "name": 'indigo',
                                        "value": '#3F51B5'
                                    }, {
                                        "name": 'blue',
                                        "value": '#2196F3'
                                    }, {
                                        "name": 'cyan',
                                        "value": '#00BCD4'
                                    }, {
                                        "name": 'orange',
                                        "value": '#FF9800'
                                    },]

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
                                            border.color: Material.background
                                            border.width: 3
                                            color: parent.color
                                            height: width
                                            radius: width / 2
                                            visible: color === MD.Token.color.accentColor
                                            width: 18
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            hoverEnabled: true

                                            onClicked: {
                                                MD.Token.color.accentColor = modelData.value;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            MD.Divider {
            }

            SettingSection {
                id: sec_play
                Layout.fillWidth: true
                title: qsTr('play')

                SettingRow {
                    Layout.fillWidth: true
                    text: qsTr('playing quality')
                    canInput: !comb_quality.popup.visible

                    actionItem: MD.ComboBox {
                        id: comb_quality
                        signal clicked

                        textRole: "text"
                        valueRole: "value"

                        model: ListModel {
                        }

                        Component.onCompleted: {
                            [{
                                    "text": qsTr('Standard'),
                                    "value": QA.SongUrlQuerier.LevelStandard
                                }, {
                                    "text": qsTr('Higher'),
                                    "value": QA.SongUrlQuerier.LevelHigher
                                }, {
                                    "text": qsTr('Exhigh'),
                                    "value": QA.SongUrlQuerier.LevelExhigh
                                }, {
                                    "text": qsTr('Lossless'),
                                    "value": QA.SongUrlQuerier.LevelLossless
                                }].map(el => model.append(el));

                            currentIndex = indexOfValue(settings_play.play_quality);
                            settings_play.play_quality = Qt.binding(() => {
                                    return comb_quality.currentValue;
                                });
                        }

                        onClicked: {
                            popup.open();
                        }
                    }
                }
            }

            MD.Divider {
            }

            SettingSection {
                id: sec_cache
                Layout.fillWidth: true
                title: qsTr('cache')

                SettingRow {
                    Layout.fillWidth: true
                    text: `${qsTr('total cache limit')}: ${Util.pretty_bytes(slider_total_cache.byteValue)}`
                    canInput: false

                    belowItem: ByteSlider {
                        id: slider_total_cache
                        Layout.fillWidth: true
                        from: 2
                        stepSize: 1
                        to: 19

                        onValueChanged: {
                            if (Math.floor(value) <= Math.floor(slider_media_cache.value))
                                slider_media_cache.value = value - 1;
                        }
                    }
                }

                SettingRow {
                    Layout.fillWidth: true
                    text: `${qsTr('media cache limit')}: ${Util.pretty_bytes(slider_media_cache.byteValue)}`
                    canInput: false

                    belowItem: ByteSlider {
                        id: slider_media_cache
                        Layout.fillWidth: true
                        from: 1
                        stepSize: 1
                        to: 18

                        onValueChanged: {
                            if (Math.floor(slider_total_cache.value) <= Math.floor(value))
                                slider_total_cache.value = value + 1;
                        }
                    }
                }
            }
        }
        Settings {
            id: settings_cache

            property real media_cache_limit
            property real total_cache_limit

            category: 'cache'

            Component.onCompleted: {
                slider_media_cache.setByteValue(media_cache_limit);
                media_cache_limit = Qt.binding(() => {
                        return slider_media_cache.byteValue;
                    });
                slider_total_cache.setByteValue(total_cache_limit);
                total_cache_limit = Qt.binding(() => {
                        return slider_total_cache.byteValue;
                    });
            }
            Component.onDestruction: {
                sync();
                QA.App.triggerCacheLimit();
            }
        }
        Settings {
            id: settings_play

            property int play_quality: QA.SongUrlQuerier.LevelExhigh

            category: 'play'

            Component.onCompleted: {
            }
        }
    }

    component ByteSlider: MD.Slider {
        readonly property real byteValue: value > 9 ? (value - 9) * m_GB : value * m_MB
        readonly property int m_GB: Math.pow(2, 30)
        readonly property int m_MB: 100 * Math.pow(2, 20)

        function setByteValue(v) {
            value = v >= m_GB ? v / m_GB + 9 : v / m_MB;
        }

        snapMode: Slider.SnapAlways
    }
    component SettingRow: ColumnLayout {
        id: s_row

        property alias actionItem: sr_action.contentItem
        property alias belowItem: sr_below.contentItem
        property alias text: sr_label.text
        property bool canInput: true

        MD.InputBlock {
            id: item_block
            when: !canInput
            target: sr_item
        }

        MD.ListItem {
            id: sr_item
            Layout.fillWidth: true

            contentItem: ColumnLayout {
                RowLayout {
                    MD.Text {
                        id: sr_label
                        Layout.fillWidth: true
                        typescale: MD.Token.typescale.title_small
                        font.capitalization: Font.Capitalize
                    }
                    MD.Control {
                        id: sr_action
                    }
                }
                MD.Control {
                    id: sr_below
                    Layout.fillWidth: true
                    visible: contentItem
                    contentItem: null
                }
            }

            Component.onCompleted: {
                if (s_row.actionItem?.clicked)
                    clicked.connect(s_row.actionItem.clicked);
            }
        }
    }
    component SettingSection: MD.Pane {
        default property alias m_children: sec_column.children
        property alias title: sec_title.text
        horizontalPadding: 0

        ColumnLayout {
            id: sec_column
            anchors.fill: parent
            spacing: 12

            MD.Text {
                id: sec_title
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                MD.MatProp.textColor: MD.Token.color.primary
                typescale: MD.Token.typescale.title_medium
                font.capitalization: Font.Capitalize
            }
        }
    }
}
