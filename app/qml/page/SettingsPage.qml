import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Templates as T
import Qcm.App as QA
import Qcm.Material as MD
import "../js/util.mjs" as Util

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
                                        border.color: MD.MatProp.backgroundColor
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
                    actionItem: MD.Switch {
                        checked: QA.Global.use_system_accent_color
                        onCheckedChanged: QA.Global.use_system_accent_color = checked
                    }
                }
                SettingRow {
                    Layout.fillWidth: true
                    text: qsTr('follow system night mode')
                    actionItem: MD.Switch {
                        checked: QA.Global.use_system_color_scheme
                        onCheckedChanged: QA.Global.use_system_color_scheme = checked
                    }
                }
            }
            MD.Divider {}

            SettingSection {
                id: sec_audio
                Layout.fillWidth: true
                title: qsTr('audio')

                SettingRow {
                    Layout.fillWidth: true
                    text: `${qsTr('Fade When Play/Pause')}: ${slider_fade.value} ms`
                    font.capitalization: Font.MixedCase
                    canInput: false
                    belowItem: MD.Slider {
                        id: slider_fade
                        Layout.fillWidth: true
                        from: 0
                        stepSize: 20
                        to: 1000
                        onMoved: {
                            QA.Global.player.fadeTime = value;
                        }
                        Component.onCompleted: {
                            value = QA.Global.player.fadeTime;
                        }
                    }
                }
                SettingRow {
                    Layout.fillWidth: true
                    text: qsTr('streaming quality')
                    canInput: !comb_playing_quality.popup.visible

                    actionItem: MD.ComboBox {
                        id: comb_playing_quality
                        signal clicked

                        textRole: "text"
                        valueRole: "value"

                        model: ListModel {}

                        Component.onCompleted: {
                            [
                                {
                                    "text": qsTr('Standard'),
                                    "value": QA.Enum.AQStandard
                                },
                                {
                                    "text": qsTr('Higher'),
                                    "value": QA.Enum.AQHigher
                                },
                                {
                                    "text": qsTr('Exhigh'),
                                    "value": QA.Enum.AQExhigh
                                },
                                {
                                    "text": qsTr('Lossless'),
                                    "value": QA.Enum.AQLossless
                                }
                            ].map(el => model.append(el));
                            currentIndex = indexOfValue(settings_audio.streaming_quality);
                            settings_audio.streaming_quality = Qt.binding(() => {
                                return comb_playing_quality.currentValue;
                            });
                        }

                        onClicked: {
                            popup.open();
                        }
                    }
                }
            }

            /*
            MD.Divider {}
            SettingSection {
                id: sec_play
                Layout.fillWidth: true
                title: qsTr('quality')
                SettingRow {
                    Layout.fillWidth: true
                    text: qsTr('cover quality')
                    canInput: !comb_cover_quality.popup.visible

                    actionItem: MD.ComboBox {
                        id: comb_cover_quality
                        signal clicked

                        textRole: "text"
                        valueRole: "value"
                        popup.width: 160

                        model: ListModel {
                        }

                        Component.onCompleted: {
                            [{
                                    "text": qsTr('400px'),
                                    "value": QA.App.Img400px
                                }, {
                                    "text": qsTr('800px'),
                                    "value": QA.App.Img800px
                                }, {
                                    "text": qsTr('1200px'),
                                    "value": QA.App.Img1200px
                                }, {
                                    "text": qsTr('Auto'),
                                    "value": QA.App.ImgAuto
                                }, {
                                    "text": qsTr('Origin(Slow)'),
                                    "value": QA.App.ImgOrigin
                                }].map(el => model.append(el));
                            currentIndex = indexOfValue(QA.Global.cover_quality);
                            currentValueChanged.connect(() => {
                                    QA.Global.cover_quality = comb_cover_quality.currentValue;
                                });
                        }

                        onClicked: {
                            popup.open();
                        }
                    }
                }
            }
*/

            MD.Divider {}

            SettingSection {
                id: sec_network
                Layout.fillWidth: true
                title: qsTr('network')

                ColumnLayout {
                    spacing: 8
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    MD.Text {
                        text: qsTr('proxy')
                        typescale: MD.Token.typescale.title_small
                        font.capitalization: Font.Capitalize
                    }
                    RowLayout {
                        MD.ComboBox {
                            id: comb_proxy
                            signal clicked

                            textRole: "text"
                            valueRole: "value"
                            popup.width: 160

                            model: ListModel {}

                            Component.onCompleted: {
                                [
                                    {
                                        "text": 'http',
                                        "value": QA.App.PROXY_HTTP
                                    },
                                    {
                                        "text": 'https',
                                        "value": QA.App.PROXY_HTTPS2
                                    },
                                    {
                                        "text": 'socks4',
                                        "value": QA.App.PROXY_SOCKS4
                                    },
                                    {
                                        "text": 'socks5',
                                        "value": QA.App.PROXY_SOCKS5
                                    },
                                    {
                                        "text": 'socks4a',
                                        "value": QA.App.PROXY_SOCKS4A
                                    },
                                    {
                                        "text": 'socks5h',
                                        "value": QA.App.PROXY_SOCKS5H
                                    }
                                ].map(el => model.append(el));
                                currentIndex = indexOfValue(settings_network.proxy_type);
                                currentValueChanged.connect(() => {
                                    settings_network.proxy_type = comb_proxy.currentValue;
                                });
                            }

                            onClicked: {
                                popup.open();
                            }
                        }

                        MD.Text {
                            text: '://'
                        }

                        MD.TextField {
                            id: item_tf_proxy
                            Layout.fillWidth: true
                            Component.onCompleted: {
                                text = settings_network.proxy_content;
                                settings_network.proxy_content = Qt.binding(() => {
                                    return item_tf_proxy.text;
                                });
                            }

                            onAccepted: focusChanged(focus)
                            onFocusChanged: settings_network.triggerProxy()
                        }
                    }
                }
                SettingRow {
                    Layout.fillWidth: true
                    text: qsTr('ignore SSL certificate')
                    actionItem: MD.Switch {
                        id: item_ignore_cert
                        checked: false
                        onCheckedChanged: QA.App.setVerifyCertificate(!checked)
                    }
                }
            }

            MD.Divider {}

            SettingSection {
                id: sec_cache
                Layout.fillWidth: true
                title: qsTr('storage')

                SettingRow {
                    Layout.fillWidth: true
                    text: qsTr('Summary')
                    canInput: false
                    supportText: m_qr_storage.status === QA.Enum.Quering ? qsTr('querying') : Util.pretty_bytes(m_qr_storage.data.total * 1024, 1)

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
                    text: `${qsTr('total size limit')}: ${Util.pretty_bytes(slider_total_cache.byteValue)}`
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
                    text: `${qsTr('music cache limit')}: ${Util.pretty_bytes(slider_media_cache.byteValue)}`
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
            MD.Divider {}
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
            id: settings_audio
            category: 'audio'
            property int streaming_quality: QA.Enum.AQExhigh
            Component.onCompleted: {}
        }
        Settings {
            id: settings_network

            property int proxy_type: QA.App.PROXY_HTTP
            property string proxy_content: ''
            property alias ignore_certificate: item_ignore_cert.checked

            category: 'network'

            function triggerProxy() {
                QA.App.setProxy(proxy_type, proxy_content);
            }

            Component.onDestruction: {
                triggerProxy();
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

        snapMode: T.Slider.SnapAlways
    }
    component SettingRow: ColumnLayout {
        id: s_row

        property alias actionItem: sr_action.contentItem
        property alias belowItem: sr_below.contentItem
        property alias text: sr_label.text
        property alias supportText: sr_label_support.text
        property alias font: sr_item.font
        property bool canInput: true

        MD.InputBlock {
            id: item_block
            when: !canInput
            target: sr_item
        }

        MD.ListItem {
            id: sr_item
            Layout.fillWidth: true
            font.capitalization: Font.Capitalize

            contentItem: ColumnLayout {
                RowLayout {
                    ColumnLayout {
                        MD.Label {
                            id: sr_label
                            Layout.fillWidth: true
                            typescale: MD.Token.typescale.title_small
                        }
                        MD.Label {
                            id: sr_label_support
                            visible: text
                            Layout.fillWidth: true
                            typescale: MD.Token.typescale.title_small
                            color: MD.MatProp.color.on_surface_variant
                        }
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
                if (s_row.actionItem?.checkable)
                    clicked.connect(s_row.actionItem.toggle);
            }
        }
    }
    component SettingSection: MD.Pane {
        default property alias m_children: sec_column.children
        property alias title: sec_title.text
        horizontalPadding: 0
        font.capitalization: Font.Capitalize

        ColumnLayout {
            id: sec_column
            anchors.fill: parent
            spacing: 12

            MD.Label {
                id: sec_title
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                MD.MatProp.textColor: MD.Token.color.primary
                typescale: MD.Token.typescale.title_medium
            }
        }
    }
}
