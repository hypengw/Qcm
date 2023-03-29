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

    title: qsTr('settings')
    font.capitalization: Font.Capitalize

    ColumnLayout {
        anchors.fill: parent

        SettingSection {
            id: sec_cache

            Layout.fillWidth: true
            title: qsTr('cache')

            ColumnLayout {
                Layout.leftMargin: 12
                Layout.rightMargin: 12

                ColumnLayout {
                    spacing: 0

                    Label {
                        text: `${qsTr('total cache limit')}: ${Util.prettyBytes(slider_total_cache.byteValue)}`
                    }

                    ByteSlider {
                        id: slider_total_cache

                        from: 2
                        to: 19
                        stepSize: 1
                        Layout.fillWidth: true
                        onValueChanged: {
                            if (Math.floor(value) <= Math.floor(slider_media_cache.value))
                                slider_media_cache.value = value - 1;

                        }
                    }

                }

                ColumnLayout {
                    spacing: 0

                    Label {
                        text: `${qsTr('media cache limit')}: ${Util.prettyBytes(slider_media_cache.byteValue)}`
                    }

                    ByteSlider {
                        id: slider_media_cache

                        from: 1
                        to: 18
                        stepSize: 1
                        Layout.fillWidth: true
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
        id: settings

        property real total_cache_limit
        property real media_cache_limit

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

    component ByteSlider: Slider {
        readonly property int m_GB: Math.pow(2, 30)
        readonly property int m_MB: 100 * Math.pow(2, 20)
        readonly property real byteValue: value > 9 ? (value - 9) * m_GB : value * m_MB

        function setByteValue(v) {
            value = v >= m_GB ? v / m_GB + 9 : v / m_MB;
        }

        snapMode: Slider.SnapAlways
    }

    component SettingSection: Pane {
        default property alias m_children: sec_column.children
        property alias title: sec_title.text

        Material.elevation: 1
        Material.background: Theme.color.surface_1
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

    component SettingRow: ColumnLayout {
        id: s_row

        property alias text: sr_label.text
        property alias actionItem: sr_action.contentItem

        MItemDelegate {

            contentItem: RowLayout {
                Label {
                    id: sr_label

                    Layout.fillWidth: true
                }

                Control {
                    id: sr_action
                }

            }

        }

    }

}
