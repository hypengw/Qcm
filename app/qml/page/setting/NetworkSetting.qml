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

        ColumnLayout {
            height: implicitHeight
            spacing: 12
            width: parent.width

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
        }
    }
}
