import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Qcm.App as QA
import Qcm.Service.Ncm as QNCM
import Qcm.Material as MD

MD.Page {
    id: root
    title: qsTr('log in')
    ColumnLayout {
        anchors.fill: parent

        readonly property bool loginOk: qr_login.data.code === 200 || qr_qrlogin.data.code === 803

        QNCM.Session {
            id: m_session
        }

        QNCM.QrcodeLoginQuerier {
            id: qr_qrlogin
            key: qr_unikey.data.key
            session: m_session

            onFinished: {
                if (data.code === 803) {
                    QA.Action.load_session(m_session);
                }
            }
        }
        QNCM.QrcodeUnikeyQuerier {
            id: qr_unikey
            session: m_session

            readonly property int loginCode: qr_qrlogin.data.code

            onLoginCodeChanged: {
                if (loginCode === 800)
                    query();
            }
        }
        QNCM.LoginQuerier {
            id: qr_login
            session: m_session
            autoReload: false

            onFinished: {
                if (data.code === 200) {
                    QA.Action.load_session(m_session);
                }
            }

            function login() {
                username = tf_username.text;
                password = QA.App.md5(tf_password.text);
                query();
            }
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: false
            Layout.preferredWidth: 300
            spacing: 12

            MD.IconSvg {
                Layout.alignment: Qt.AlignHCenter
                source: 'qrc:/Qcm/Service/Ncm/assets/netease.svg'
                size: 128
            }

            MD.Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr('log into netease')
                font.capitalization: Font.Capitalize
                typescale: MD.Token.typescale.title_large
            }

            MD.TabBar {
                id: bar
                Layout.fillWidth: true
                font.capitalization: Font.Capitalize

                onCurrentIndexChanged: {
                    view.currentIndex = currentIndex;
                }

                MD.TabButton {
                    text: qsTr("email")
                }
                MD.TabButton {
                    text: qsTr("QR code")
                }
            }
            SwipeView {
                id: view
                Layout.fillWidth: true
                clip: true
                implicitHeight: Math.max(mail_pane.implicitHeight, qr_pane.implicitHeight)

                onCurrentIndexChanged: {
                    bar.currentIndex = currentIndex;
                }

                MD.Pane {
                    id: mail_pane
                    ColumnLayout {
                        anchors.fill: parent

                        MD.TextField {
                            id: tf_username
                            Layout.fillWidth: true
                            placeholderText: 'Email'
                        }
                        MD.TextField {
                            id: tf_password
                            Layout.fillWidth: true
                            echoMode: TextInput.Password
                            placeholderText: 'Password'
                        }
                        MD.Text {
                            MD.MatProp.textColor: MD.Token.color.error
                            text: {
                                switch (qr_login.data.code) {
                                case 501:
                                    return qsTr('Email not found');
                                case 502:
                                    return qsTr('Invalid email or password');
                                default:
                                    return '';
                                }
                            }
                        }
                        MD.Button {
                            Layout.fillWidth: true
                            enabled: qr_login.status !== QA.enums.Querying
                            font.capitalization: Font.Capitalize
                            highlighted: true
                            text: qsTr('log in')

                            Component.onCompleted: {
                                tf_username.accepted.connect(clicked);
                                tf_password.accepted.connect(clicked);
                            }
                            onClicked: {
                                qr_login.login();
                            }
                        }
                    }
                }
                MD.Pane {
                    id: qr_pane
                    padding: 8

                    MD.StackView {
                        id: qr_stack

                        property QtObject cur: switch (qr_qrlogin.data.code) {
                        case 803:
                        case 802:
                            return comp_qr_wait_comfirm;
                        default:
                            return comp_qr_wait_scan;
                        }

                        anchors.fill: parent
                        implicitHeight: 224

                        onCurChanged: {
                            replace(currentItem, cur);
                        }
                    }
                    Component {
                        id: comp_qr_wait_scan
                        ColumnLayout {
                            MD.Pane {
                                Layout.alignment: Qt.AlignCenter
                                backgroundColor: 'white'
                                elevation: MD.Token.elevation.level3
                                padding: 12
                                radius: 12
                                MD.Image {
                                    id: qr_image
                                    anchors.centerIn: parent
                                    cache: false
                                    source: `image://qr/${qr_unikey.data.qrurl}`
                                    sourceSize.height: 200
                                    sourceSize.width: 200
                                }
                            }
                        }
                    }
                    Component {
                        id: comp_qr_wait_comfirm
                        ColumnLayout {
                            MD.Image {
                                Layout.alignment: Qt.AlignHCenter

                                source: QA.Util.image_url(qr_qrlogin.data.avatarUrl)
                                sourceSize.height: 96
                                sourceSize.width: 96
                                radius: height / 2
                            }
                            MD.Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: qr_qrlogin.data.nickname
                            }
                            MD.Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: qr_qrlogin.data.message
                            }
                        }
                    }
                    Timer {
                        interval: 2000
                        repeat: true
                        running: qr_pane.SwipeView.isCurrentItem
                        triggeredOnStart: true

                        onTriggered: {
                            qr_qrlogin.query();
                        }
                    }
                }
            }
        }
    }
}
