import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Qt.labs.settings
import QcmApp
import ".."
import "../component"
import "../part"
import "../js/util.mjs" as Util

Page {
    id: root
    font.capitalization: Font.Capitalize
    title: qsTr('settings')

    MFlickable {
        id: flick
        anchors.fill: parent
        leftMargin: 0
        rightMargin: 0

        //   ScrollBar.vertical: ScrollBar {}
        ColumnLayout {
            height: implicitHeight
            spacing: 12
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

                        Label {
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
                                            visible: color === Theme.color.accentColor
                                            width: 18
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            hoverEnabled: true

                                            onClicked: {
                                                Theme.color.accentColor = modelData.value;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            SettingSection {
                id: sec_play
                Layout.fillWidth: true
                title: qsTr('play')

                SettingRow {
                    Layout.fillWidth: true
                    text: qsTr('playing quality')

                    actionItem: ComboBox {
                        id: comb_quality
                        signal clicked

                        popup.modal: true
                        textRole: "text"
                        valueRole: "value"

                        model: ListModel {
                            ListElement {
                                text: qsTr('Standard')
                                value: SongUrlQuerier.LevelStandard
                            }
                            ListElement {
                                text: qsTr('Higher')
                                value: SongUrlQuerier.LevelHigher
                            }
                            ListElement {
                                text: qsTr('Exhigh')
                                value: SongUrlQuerier.LevelExhigh
                            }
                            ListElement {
                                text: qsTr('Lossless')
                                value: SongUrlQuerier.LevelLossless
                            }
                        }

                        onClicked: {
                            popup.open();
                        }
                    }
                }
            }
            SettingSection {
                id: sec_cache
                Layout.fillWidth: true
                title: qsTr('cache')

                ColumnLayout {
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    ColumnLayout {
                        spacing: 0

                        Label {
                            text: `${qsTr('total cache limit')}: ${Util.pretty_bytes(slider_total_cache.byteValue)}`
                        }
                        ByteSlider {
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
                    ColumnLayout {
                        spacing: 0

                        Label {
                            text: `${qsTr('media cache limit')}: ${Util.pretty_bytes(slider_media_cache.byteValue)}`
                        }
                        ByteSlider {
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
                App.triggerCacheLimit();
            }
        }
        Settings {
            id: settings_play

            property int play_quality: SongUrlQuerier.LevelExhigh

            category: 'play'

            Component.onCompleted: {
                comb_quality.currentIndex = comb_quality.indexOfValue(play_quality);
                play_quality = Qt.binding(() => {
                        return comb_quality.currentValue;
                    });
            }
        }
    }

    component ByteSlider: Slider {
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
        property alias text: sr_label.text

        MItemDelegate {
            Layout.fillWidth: true

            contentItem: RowLayout {
                Label {
                    id: sr_label
                    Layout.fillWidth: true
                }
                Control {
                    id: sr_action
                }
            }

            Component.onCompleted: {
                if (actionItem.clicked)
                    clicked.connect(actionItem.clicked);
            }
        }
    }
    component SettingSection: Pane {
        default property alias m_children: sec_column.children
        property alias title: sec_title.text

        Material.background: Theme.color.surface_container_lowest
        Material.elevation: 1
        horizontalPadding: 0

        ColumnLayout {
            id: sec_column
            anchors.fill: parent
            spacing: 12

            Label {
                id: sec_title
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                Material.foreground: Theme.color.primary
            }
        }
    }
}
