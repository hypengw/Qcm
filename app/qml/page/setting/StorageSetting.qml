import QtCore
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

        ColumnLayout {
            height: implicitHeight
            spacing: 12
            width: parent.width

            SettingSection {
                id: sec_cache
                Layout.fillWidth: true
                title: qsTr('storage')

                SettingRow {
                    Layout.fillWidth: true
                    text: qsTr('Summary')
                    canInput: false
                    supportText: m_qr_storage.status === QA.Enum.Quering ? qsTr('querying') : QA.Util.prettyBytes(m_qr_storage.data.total * 1024, 1)

                    QA.StorageInfoQuerier {
                        id: m_qr_storage

                        Component.onCompleted: reload()
                    }

                    actionItem: MD.Button {
                        type: MD.Enum.BtText
                        text: qsTr('clear all')
                        onClicked: {
                            QA.Action.toast('work in progress');
                        }
                    }
                }

                SettingRow {
                    Layout.fillWidth: true
                    text: `${qsTr('total size limit')}: ${QA.Util.prettyBytes(slider_total_cache.byteValue)}`
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
                    text: `${qsTr('music cache limit')}: ${QA.Util.prettyBytes(slider_media_cache.byteValue)}`
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
    }
}
