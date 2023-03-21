import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QcmApp
import ".."
import "../component"

ColumnLayout {
    id: root

    readonly property bool loginCodeOk: qr_login.data.code === 200 || qr_qrlogin.data.code === 803

    onLoginCodeOkChanged: {
        QA.querier_user.query();
    }

    ApiContainer {
        QrcodeLoginQuerier {
            id: qr_qrlogin

            key: qr_unikey.data.key
        }

        QrcodeUnikeyQuerier {
            id: qr_unikey

            readonly property int loginCode: qr_qrlogin.data.code

            onLoginCodeChanged: {
                if (loginCode === 800)
                    query();

            }
        }

        LoginQuerier {
            id: qr_login

            function login() {
                username = tf_username.text;
                password = App.md5(tf_password.text);
                query();
            }

            autoReload: false
        }

    }

    ColumnLayout {
        Layout.alignment: Qt.AlignHCenter
        Layout.fillWidth: false
        Layout.preferredWidth: 300

        Label {
            Layout.fillWidth: true
            Layout.bottomMargin: 20
            text: qsTr('Login')
            font.capitalization: Font.Capitalize
            font.pointSize: 18
        }

        TabBar {
            id: bar

            Layout.fillWidth: true
            onCurrentIndexChanged: {
                view.currentIndex = currentIndex;
            }

            TabButton {
                text: qsTr("Email")
            }

            TabButton {
                text: qsTr("Qr")
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

            Pane {
                id: mail_pane

                ColumnLayout {
                    anchors.fill: parent

                    TextField {
                        id: tf_username

                        Layout.fillWidth: true
                        placeholderText: 'email'
                    }

                    TextField {
                        id: tf_password

                        Layout.fillWidth: true
                        placeholderText: 'password'
                        echoMode: TextInput.Password
                    }

                    Label {
                        Material.foreground: Theme.color.error
                        text: {
                            switch (qr_login.data.code) {
                            case 501:
                                return qsTr('email not exists');
                            case 502:
                                return qsTr('wrong password');
                            default:
                                return '';
                            }
                        }
                    }

                    Button {
                        Layout.fillWidth: true
                        text: qsTr('login in')
                        font.capitalization: Font.Capitalize
                        highlighted: true
                        enabled: qr_login.status !== ApiQuerierBase.Querying
                        onClicked: {
                            qr_login.login();
                        }
                    }

                }

            }

            Pane {
                id: qr_pane

                clip: true

                StackView {
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
                        Pane {
                            Layout.alignment: Qt.AlignCenter
                            Material.elevation: 4
                            Material.background: 'white'
                            padding: 12

                            Image {
                                id: qr_image

                                anchors.centerIn: parent
                                sourceSize.width: 200
                                sourceSize.height: 200
                                source: `image://qr/${qr_unikey.data.qrurl}`
                                cache: false
                            }

                        }

                    }

                }

                Component {
                    id: comp_qr_wait_comfirm

                    ColumnLayout {
                        RoundImage {
                            Layout.alignment: Qt.AlignHCenter

                            image: Image {
                                sourceSize.width: 96
                                sourceSize.height: 96
                                source: `image://ncm/${qr_qrlogin.data.avatarUrl}`
                            }

                        }

                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: qr_qrlogin.data.nickname
                        }

                        Label {
                            Layout.alignment: Qt.AlignHCenter
                            text: qr_qrlogin.data.message
                        }

                    }

                }

                Timer {
                    running: qr_pane.SwipeView.isCurrentItem
                    interval: 2000
                    repeat: true
                    triggeredOnStart: true
                    onTriggered: {
                        qr_qrlogin.query();
                    }
                }

            }

        }

    }

}
