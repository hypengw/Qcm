pragma ComponentBehavior: Bound
import QtCore
import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

QA.SettingBasePage {
    id: root
    title: qsTr('storage')

    QA.StorageInfoQuery {
        id: m_qr
        Component.onCompleted: reload()
    }

    component SizePane: MD.Pane {
        id: m_root
        Layout.fillWidth: true
        backgroundColor: MD.Token.color.surface_container
        property alias title: m_title.text
        property real size: 0
        property Item action: null

        padding: 8
        horizontalPadding: 12
        radius: MD.Token.shape.corner.medium

        RowLayout {
            width: parent.width
            Column {
                Layout.fillWidth: true
                spacing: 4
                MD.Label {
                    id: m_title
                }
                MD.Loader {
                    sourceComponent: m_qr.querying ? m_comp_loading : m_comp_size
                }
                Component {
                    id: m_comp_size
                    MD.Text {
                        typescale: MD.Token.typescale.title_medium
                        text: QA.Util.prettyBytes(m_root.size, 1)
                    }
                }
                Component {
                    id: m_comp_loading
                    MD.CircularIndicator {
                        anchors.centerIn: parent
                        running: true
                        strokeWidth: 2
                        padding: 0
                        implicitWidth: 24
                        implicitHeight: 24
                        color: MD.Token.color.primary
                    }
                }
            }

            LayoutItemProxy {
                target: m_root.action
            }
        }
    }

    ColumnLayout {
        id: m_content
        spacing: 12
        width: parent.width
        height: implicitHeight

        onYChanged: console.log("y changed")

        Column {
            Layout.leftMargin: 4
            MD.Label {
                text: qsTr('used space')
            }
            MD.Text {
                typescale: MD.Token.typescale.headline_small
                text: m_qr.querying ? qsTr('calculating...') : QA.Util.prettyBytes(m_qr.data.total, 1)
            }
        }
        SizePane {
            title: qsTr('media cache')
            size: m_qr.data.media
            action: MD.Button {
                type: MD.Enum.BtFilled
                text: qsTr('clear')
                background.implicitHeight: 36
                onClicked: {
                    QA.Action.toast('work in progress');
                }
            }
        }
        SizePane {
            title: qsTr('image cache')
            size: m_qr.data.image
            action: MD.Button {
                type: MD.Enum.BtFilled
                text: qsTr('clear')
                background.implicitHeight: 36
                onClicked: {
                    QA.Action.toast('work in progress');
                }
            }
        }
        SizePane {
            title: qsTr('database')
            size: m_qr.data.database
        }
    }

    /*
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
        */

}
